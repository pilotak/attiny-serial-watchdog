#pragma once
#include "config.h"

// PA7=RX always, PA6=TX only if USE_SERIAL_TX
static inline void usartInit() {
#ifdef DAC0
  DAC0.CTRLA = 0;
#endif
  USART0.BAUD = (uint16_t)(F_CPU * 4UL / SERIAL_BAUD);
  USART0.CTRLC = USART_CMODE_ASYNCHRONOUS_gc | USART_PMODE_DISABLED_gc | USART_SBMODE_1BIT_gc | USART_CHSIZE_8BIT_gc;
#ifdef USE_SERIAL_TX
  PORTA.DIRSET = PIN6_bm;  // TX pin as output
  USART0.CTRLB = USART_RXEN_bm | USART_TXEN_bm;
#else
  USART0.CTRLB = USART_RXEN_bm;
#endif
}

#ifdef USE_SERIAL_TX
static inline void uartSendByte(uint8_t c) {
  while (!(USART0.STATUS & USART_DREIF_bm));
  USART0.TXDATAL = c;
}

static inline void uartSendStr(const char *s) {
  while (*s) uartSendByte((uint8_t)*s++);
}

static inline void uartSendNum(uint16_t v) {
  char buf[6];
  uint8_t len = 0;
  if (v == 0) { uartSendByte('0'); return; }
  while (v) { buf[len++] = '0' + (v % 10); v /= 10; }
  for (int8_t j = len - 1; j >= 0; j--) uartSendByte((uint8_t)buf[j]);
}

static inline void uartSendInt(int16_t v) {
  if (v < 0) { uartSendByte('-'); uartSendNum((uint16_t)(-v)); }
  else uartSendNum((uint16_t)v);
}
#endif
