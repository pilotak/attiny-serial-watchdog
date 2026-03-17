#pragma once
#include <Arduino.h>
#include <avr/pgmspace.h>

// --- Pin definitions ---
#define PIN_RELAY1 PIN_PA3
#define PIN_RELAY2 PIN_PA1
#define PIN_RELAY3 PIN_PA2
#define PIN_TEMP PIN_PA6  // NTC (ADC) or DS18B20 (OneWire)

// --- Relay indexing (0-based internal, 1-based in commands) ---
#define NUM_RELAYS 3
static const uint8_t relayPins[NUM_RELAYS] = {PIN_RELAY1, PIN_RELAY2, PIN_RELAY3};

// --- Relay modes ---
#define MODE_MANUAL 0
#define MODE_WATCHDOG 1
#define MODE_FAN 2
#define MODE_WATCHDOG_INV 3  // inverse: OFF normally, turns ON on timeout

// --- Temperature sensor detection ---
#if defined(USE_NTC) || defined(USE_DS18B20)
  #define HAS_TEMP_SENSOR
#endif

// --- Defaults ---
#define DEFAULT_WD_TIMEOUT 300      // seconds
#define DEFAULT_WD_RESTART_DELAY 5  // seconds relay stays OFF before coming back on
#define DEFAULT_TARGET_TEMP 40      // C
#define DEFAULT_HYSTERESIS 3        // C

// --- EEPROM layout ---
#define EEPROM_MAGIC 0xA7
#define EE_MAGIC_ADDR 0
#define EE_MODES_ADDR 1
#define EE_WD_TIMEOUT_ADDR 2        // uint16_t, 2 bytes
#define EE_WD_RESTART_DELAY_ADDR 4  // uint16_t, 2 bytes
#define EE_TEMP_TGT_ADDR 6          // int8_t
#define EE_HYSTERESIS_ADDR 7        // uint8_t

// --- NTC lookup table (only used with USE_NTC) ---
#ifdef USE_NTC
  // ADC values for -30C to 110C, 10C steps
  // B=3950, R0=10k@25C, Rpullup=10k, 10-bit ADC
  // Hot = low ADC, Cold = high ADC
  #define NTC_TABLE_SIZE 15
  #define NTC_TEMP_MIN (-30)
  #define NTC_TEMP_STEP 10
static const uint16_t ntcTable[NTC_TABLE_SIZE] PROGMEM = {
    // -30  -20  -10    0   10   20   30   40   50   60   70   80   90  100  110
    974, 934, 873, 788, 684, 569, 456, 354, 270, 204, 153, 115, 87, 67, 51};
#endif

// --- Timing ---
#define TEMP_READ_INTERVAL 1000  // ms
#define TX_STATUS_INTERVAL 2000  // ms

// --- Serial ---
#define CMD_BUF_SIZE 16
#define SERIAL_BAUD 9600
