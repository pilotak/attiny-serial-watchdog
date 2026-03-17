#pragma once
#include "config.h"

#ifdef USE_NTC

inline uint16_t tempToADC(int8_t tempC) {
  if (tempC <= NTC_TEMP_MIN) return pgm_read_word(&ntcTable[0]);
  int8_t maxTemp = NTC_TEMP_MIN + (NTC_TABLE_SIZE - 1) * NTC_TEMP_STEP;
  if (tempC >= maxTemp) return pgm_read_word(&ntcTable[NTC_TABLE_SIZE - 1]);

  int16_t offset = tempC - NTC_TEMP_MIN;
  uint8_t idx = offset / NTC_TEMP_STEP;
  uint8_t frac = offset % NTC_TEMP_STEP;

  uint16_t a = pgm_read_word(&ntcTable[idx]);
  uint16_t b = pgm_read_word(&ntcTable[idx + 1]);

  return a - (uint16_t)((uint32_t)(a - b) * frac / NTC_TEMP_STEP);
}

#endif
