#pragma once
#include "config.h"

#ifdef USE_DS18B20

// Minimal bit-bang OneWire for single DS18B20 on PIN_TEMP
inline void owReset() {
  pinMode(PIN_TEMP, OUTPUT);
  digitalWrite(PIN_TEMP, LOW);
  delayMicroseconds(480);
  pinMode(PIN_TEMP, INPUT);
  delayMicroseconds(480);
}

inline void owWriteBit(uint8_t bit) {
  pinMode(PIN_TEMP, OUTPUT);
  digitalWrite(PIN_TEMP, LOW);
  if (bit) {
    delayMicroseconds(5);
    pinMode(PIN_TEMP, INPUT);
    delayMicroseconds(60);
  } else {
    delayMicroseconds(60);
    pinMode(PIN_TEMP, INPUT);
    delayMicroseconds(5);
  }
}

inline uint8_t owReadBit() {
  pinMode(PIN_TEMP, OUTPUT);
  digitalWrite(PIN_TEMP, LOW);
  delayMicroseconds(2);
  pinMode(PIN_TEMP, INPUT);
  delayMicroseconds(10);
  uint8_t b = digitalRead(PIN_TEMP);
  delayMicroseconds(50);
  return b;
}

inline void owWriteByte(uint8_t data) {
  for (uint8_t i = 0; i < 8; i++) {
    owWriteBit(data & 0x01);
    data >>= 1;
  }
}

inline uint8_t owReadByte() {
  uint8_t data = 0;
  for (uint8_t i = 0; i < 8; i++) {
    data >>= 1;
    if (owReadBit()) data |= 0x80;
  }
  return data;
}

inline void ds18b20StartConversion() {
  owReset();
  owWriteByte(0xCC); // Skip ROM
  owWriteByte(0x44); // Convert T
}

inline int8_t ds18b20Read() {
  owReset();
  owWriteByte(0xCC); // Skip ROM
  owWriteByte(0xBE); // Read Scratchpad
  uint8_t lsb = owReadByte();
  uint8_t msb = owReadByte();
  int16_t raw = (int16_t)((msb << 8) | lsb);
  return (int8_t)(raw >> 4); // 12-bit to whole degrees
}

#endif
