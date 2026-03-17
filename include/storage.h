#pragma once
#include "config.h"
#include <EEPROM.h>

extern uint8_t relayModes[NUM_RELAYS];
extern uint16_t wdTimeout;
extern uint16_t wdRestartDelay;
#ifdef HAS_TEMP_SENSOR
extern int8_t targetTemp;
extern uint8_t hysteresis;
#endif

static inline void eepromLoad() {
  if (EEPROM.read(EE_MAGIC_ADDR) != EEPROM_MAGIC) {
    relayModes[0] = MODE_MANUAL;
    relayModes[1] = MODE_MANUAL;
    relayModes[2] = MODE_MANUAL;
    wdTimeout = DEFAULT_WD_TIMEOUT;
    wdRestartDelay = DEFAULT_WD_RESTART_DELAY;
#ifdef HAS_TEMP_SENSOR
    targetTemp = DEFAULT_TARGET_TEMP;
    hysteresis = DEFAULT_HYSTERESIS;
#endif
    return;
  }
  uint8_t packed = EEPROM.read(EE_MODES_ADDR);
  relayModes[0] = packed & 0x03;
  relayModes[1] = (packed >> 2) & 0x03;
  relayModes[2] = (packed >> 4) & 0x03;

  EEPROM.get(EE_WD_TIMEOUT_ADDR, wdTimeout);
  EEPROM.get(EE_WD_RESTART_DELAY_ADDR, wdRestartDelay);
#ifdef HAS_TEMP_SENSOR
  targetTemp = (int8_t)EEPROM.read(EE_TEMP_TGT_ADDR);
  hysteresis = EEPROM.read(EE_HYSTERESIS_ADDR);
#endif
}

static inline void eepromSave() {
  EEPROM.update(EE_MAGIC_ADDR, EEPROM_MAGIC);
  uint8_t packed = (relayModes[0] & 0x03) | ((relayModes[1] & 0x03) << 2) | ((relayModes[2] & 0x03) << 4);
  EEPROM.update(EE_MODES_ADDR, packed);
  EEPROM.put(EE_WD_TIMEOUT_ADDR, wdTimeout);
  EEPROM.put(EE_WD_RESTART_DELAY_ADDR, wdRestartDelay);
#ifdef HAS_TEMP_SENSOR
  EEPROM.update(EE_TEMP_TGT_ADDR, (uint8_t)targetTemp);
  EEPROM.update(EE_HYSTERESIS_ADDR, hysteresis);
#endif
}
