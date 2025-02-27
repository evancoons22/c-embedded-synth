#ifndef EMBEDDED_DELAY_H
#define EMBEDDED_DELAY_H

#ifdef ARDUINO
  #include <Arduino.h>
  static inline void embedded_delay_ms(uint32_t ms) { delay(ms); }
#else
  #include <unistd.h>
  static inline void embedded_delay_ms(uint32_t ms) { usleep(ms * 1000); }
#endif

#endif

