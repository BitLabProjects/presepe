#ifndef _OS_H_
#define _OS_H_

#include <stdio.h>
#include <stdarg.h>

class Os {
public:
  static int getUsedHeap();
  
  static inline void debug(const char *format, ...)
  {
  #if 1
      va_list args;
      va_start(args, format);
      vfprintf(stderr, format, args);
      va_end(args);
  #endif
  }

  static inline void assertFalse(const char *msg)
  {
    debug("Assert failed: %s\n", msg);
  }
};

#endif