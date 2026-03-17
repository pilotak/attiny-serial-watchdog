#include "Arduino.h"
#include "config.h"
#include "serial.h"
#include "storage.h"

#ifdef USE_NTC
  #include "ntc.h"
#elif defined(USE_DS18B20)
  #include "ds18b20.h"
#endif

// --- Global state ---
uint8_t relayModes[NUM_RELAYS];
uint8_t relayState[NUM_RELAYS];

uint16_t wdTimeout = DEFAULT_WD_TIMEOUT;
uint16_t wdRestartDelay = DEFAULT_WD_RESTART_DELAY;
uint32_t wdLastPing[NUM_RELAYS];
uint32_t wdOffTime[NUM_RELAYS];  // millis() when relay was cut by watchdog (0 = not tripped)

#ifdef HAS_TEMP_SENSOR
int8_t targetTemp = DEFAULT_TARGET_TEMP;
uint8_t hysteresis = DEFAULT_HYSTERESIS;

  #ifdef USE_NTC
uint16_t fanOnADC;
uint16_t fanOffADC;
  #else
int8_t fanOnTemp;
int8_t fanOffTemp;
  #endif

uint32_t lastTempRead;
#elif defined(USE_SERIAL_TX)
uint32_t lastStatus;  // Used for periodic serial TX output
#endif

// Manual relay auto-off timers
uint32_t relayOffTime[NUM_RELAYS];  // millis when to turn off (0 = no timer)

// Serial command buffer
char cmdBuf[CMD_BUF_SIZE];
uint8_t cmdLen;

#ifdef HAS_TEMP_SENSOR
void updateFanThresholds() {
  #ifdef USE_NTC
  fanOnADC = tempToADC(targetTemp + hysteresis);
  fanOffADC = tempToADC(targetTemp - hysteresis);
  #else
  fanOnTemp = targetTemp + hysteresis;
  fanOffTemp = targetTemp - hysteresis;
  #endif
}
#endif

// --- Set relay output ---
void setRelay(uint8_t idx, uint8_t on) {
  relayState[idx] = on;
  digitalWrite(relayPins[idx], on ? HIGH : LOW);
}

// --- Parse number from string position ---
int32_t parseNum(const char *s) {
  bool neg = (*s == '-');
  if (neg) {
    s++;
  }
  int32_t v = 0;
  while (*s >= '0' && *s <= '9') {
    v = v * 10 + (*s - '0');
    s++;
  }
  return neg ? -v : v;
}

// --- Try to parse an assignment command (X=value), returns value or INT32_MIN on bad format ---
#define PARSE_NONE INT32_MIN

int32_t parseAssignment() {
  if (cmdLen >= 3 && cmdBuf[1] == '=') {
    return parseNum(&cmdBuf[2]);
  }
  return PARSE_NONE;
}

// --- Set relay mode, reset timers, set initial state, save ---
void setRelayMode(uint8_t idx, uint8_t mode, uint8_t initialState) {
  relayModes[idx] = mode;
  relayOffTime[idx] = 0;
  wdOffTime[idx] = 0;
  if (mode == MODE_WATCHDOG || mode == MODE_WATCHDOG_INV) {
    wdLastPing[idx] = millis();
  }
  setRelay(idx, initialState);
  eepromSave();
}

// --- Handle Rn= relay commands ---
void handleRelayCmd() {
  if (cmdLen < 4 || cmdBuf[2] != '=') {
    return;
  }

  uint8_t idx = cmdBuf[1] - '1';
  if (idx >= NUM_RELAYS) {
    return;
  }

  // Mode assignment: Rn=W (watchdog), I (inv watchdog), F (fan), M (manual)
  char c3 = cmdBuf[3];
  switch (c3) {
    case 'W':
      setRelayMode(idx, MODE_WATCHDOG, 1);
      return;
    case 'I':
      setRelayMode(idx, MODE_WATCHDOG_INV, 0);
      return;
    case 'F':
      setRelayMode(idx, MODE_FAN, 0);
      return;
    case 'M':
      setRelayMode(idx, MODE_MANUAL, 0);
      return;
  }

  // Manual control: Rn=ON, Rn=OFF, Rn=<seconds> (only when in manual mode)
  if (relayModes[idx] != MODE_MANUAL) {
    return;
  }

  if (c3 == 'O' && cmdLen >= 5) {  // Rn=ON or Rn=OFF
    setRelay(idx, cmdBuf[4] == 'N');
    relayOffTime[idx] = 0;
  } else if (c3 >= '0' && c3 <= '9') {  // Rn=<seconds> timed ON
    uint16_t secs = parseNum(&cmdBuf[3]);
    setRelay(idx, 1);
    if (secs > 0) {
      relayOffTime[idx] = millis() + (uint32_t)secs * 1000UL;
      if (relayOffTime[idx] == 0) {
        relayOffTime[idx] = 1;  // avoid 0 (means "no timer")
      }
    } else {
      relayOffTime[idx] = 0;
    }
  }
}

// --- Process a complete command ---
void processCommand() {
  if (cmdLen == 0) {
    return;
  }
  cmdBuf[cmdLen] = '\0';

  int32_t v;

  switch (cmdBuf[0]) {
    case 'P':  // Ping — reset watchdog timer, restore tripped relays
      for (uint8_t i = 0; i < NUM_RELAYS; i++) {
        if (relayModes[i] == MODE_WATCHDOG || relayModes[i] == MODE_WATCHDOG_INV) {
          uint8_t idle = (relayModes[i] == MODE_WATCHDOG);
          wdLastPing[i] = millis();
          wdOffTime[i] = 0;
          if (relayState[i] != idle) {
            setRelay(i, idle);
          }
        }
      }
#ifdef USE_SERIAL_TX
      uartSendStr("PONG\r\n");
#endif
      break;

#ifdef HAS_TEMP_SENSOR
    case 'T':  // T=nn target temperature
      v = parseAssignment();
      if (v != PARSE_NONE && v >= -40 && v <= 125) {
        targetTemp = (int8_t)v;
        updateFanThresholds();
        eepromSave();
      }
      break;

    case 'H':  // H=nn hysteresis
      v = parseAssignment();
      if (v != PARSE_NONE && v >= 1 && v <= 20) {
        hysteresis = (uint8_t)v;
        updateFanThresholds();
        eepromSave();
      }
      break;
#endif

    case 'W':  // W=nnnn watchdog timeout (seconds)
      v = parseAssignment();
      if (v != PARSE_NONE && v >= 1 && v <= 65535) {
        wdTimeout = (uint16_t)v;
        // Reset timer so new timeout counts from now, not from last ping
        for (uint8_t i = 0; i < NUM_RELAYS; i++) {
          if (relayModes[i] == MODE_WATCHDOG || relayModes[i] == MODE_WATCHDOG_INV) {
            wdLastPing[i] = millis();
          }
        }
        eepromSave();
      }
      break;

    case 'D':  // D=nn watchdog restart delay (seconds)
      v = parseAssignment();
      if (v != PARSE_NONE && v >= 0 && v <= 65535) {
        wdRestartDelay = (uint16_t)v;
        eepromSave();
      }
      break;

    case 'R':  // Rn=... relay mode or manual control
      handleRelayCmd();
      break;
  }
}

// --- Main ---
void setup() {
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    pinMode(relayPins[i], OUTPUT);
  }

  eepromLoad();
#ifdef HAS_TEMP_SENSOR
  updateFanThresholds();
#endif
  usartInit();

#ifdef USE_SERIAL_TX
  uartSendStr("BOOT\r\n");
  // Relay modes: "M1=W,2=M,3=F"
  static const char modeChr[] = {'M', 'W', 'F', 'I'};
  uartSendByte('M');
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    if (i > 0) uartSendByte(',');
    uartSendByte('1' + i);
    uartSendByte('=');
    uartSendByte(modeChr[relayModes[i]]);
  }
  uartSendStr("\r\n");
  // Watchdog config: "W=30,D=5"
  uartSendStr("W=");
  uartSendNum(wdTimeout);
  uartSendStr(",D=");
  uartSendNum(wdRestartDelay);
  uartSendStr("\r\n");
  #ifdef HAS_TEMP_SENSOR
  // Temperature config: "T=40,H=3"
  uartSendStr("T=");
  uartSendInt(targetTemp);
  uartSendStr(",H=");
  uartSendNum(hysteresis);
  uartSendStr("\r\n");
  #endif
#endif

#ifdef USE_DS18B20
  ds18b20StartConversion();
#endif

  // Initialize relays: watchdog relays start ON, others OFF
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    wdOffTime[i] = 0;
    wdLastPing[i] = millis();
    setRelay(i, relayModes[i] == MODE_WATCHDOG ? 1 : 0);  // WATCHDOG_INV and others start OFF
  }
#ifdef HAS_TEMP_SENSOR
  lastTempRead = millis();
#elif defined(USE_SERIAL_TX)
  lastStatus = millis();
#endif
  cmdLen = 0;
}

void loop() {
  uint32_t now = millis();

  // Poll Serial RX
  while (USART0.STATUS & USART_RXCIF_bm) {
    uint8_t ch = USART0.RXDATAL;
    if (ch == '\n' || ch == '\r') {
      if (cmdLen > 0) {
        processCommand();
        cmdLen = 0;
      }
    } else if (cmdLen < CMD_BUF_SIZE - 1) {
      cmdBuf[cmdLen++] = (char)ch;
    }
  }

  // Per-relay checks: watchdog, inverse watchdog, manual auto-off
  for (uint8_t i = 0; i < NUM_RELAYS; i++) {
    switch (relayModes[i]) {
      case MODE_WATCHDOG:
      case MODE_WATCHDOG_INV: {
        uint8_t idle = (relayModes[i] == MODE_WATCHDOG);
        if (wdOffTime[i] != 0) {
          // Relay is tripped — restore to idle after restart delay
          if ((now - wdOffTime[i]) >= (uint32_t)wdRestartDelay * 1000UL) {
            wdOffTime[i] = 0;
            wdLastPing[i] = now;
            setRelay(i, idle);
          }
        } else if ((now - wdLastPing[i]) >= (uint32_t)wdTimeout * 1000UL) {
          if (relayState[i] == idle) {
            setRelay(i, !idle);
            wdOffTime[i] = now ? now : 1;
          }
        }
        break;
      }
      case MODE_MANUAL:
        if (relayOffTime[i] != 0 && (long)(now - relayOffTime[i]) >= 0) {
          setRelay(i, 0);
          relayOffTime[i] = 0;
        }
        break;
    }
  }

#ifdef HAS_TEMP_SENSOR
  // Fan/temperature control
  if ((now - lastTempRead) >= TEMP_READ_INTERVAL) {
    lastTempRead = now;

  #ifdef USE_NTC
    uint16_t adc = analogRead(PIN_TEMP);
    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
      if (relayModes[i] == MODE_FAN) {
        if (adc <= fanOnADC) {
          if (!relayState[i]) {
            setRelay(i, 1);
          }
        } else if (adc >= fanOffADC) {
          if (relayState[i]) {
            setRelay(i, 0);
          }
        }
      }
    }
  #else
    int8_t temp = ds18b20Read();
    ds18b20StartConversion();
    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
      if (relayModes[i] == MODE_FAN) {
        if (temp >= fanOnTemp) {
          if (!relayState[i]) {
            setRelay(i, 1);
          }
        } else if (temp <= fanOffTemp) {
          if (relayState[i]) {
            setRelay(i, 0);
          }
        }
      }
    }
  #endif
  }
#endif

#ifdef USE_SERIAL_TX
  // Serial TX: periodic status output
  if ((now - lastStatus) >= TX_STATUS_INTERVAL) {
    lastStatus = now;

    // Send relay status: "S1=0,2=1,3=1"
    uartSendByte('S');
    for (uint8_t i = 0; i < NUM_RELAYS; i++) {
      if (i > 0) {
        uartSendByte(',');
      }
      uartSendByte('1' + i);
      uartSendByte('=');
      uartSendByte('0' + relayState[i]);
    }
    uartSendStr("\r\n");
  }
#endif
}
