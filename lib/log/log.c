#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include "log.h"

void logDebug(char* fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  if (getenv("CIRC_DEBUG")) {
    logInfo(fmt, argptr);
  }
  va_end(argptr);
}

void logInfo(char* fmt, ...) {
  va_list argptr;
  va_start(argptr, fmt);
  vprintf(fmt, argptr);
  va_end(argptr);
}
