#include "../src/config/config.h"
#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
log_debug (char* fmt, ...)
{
	if (DEBUG) {
		va_list argptr;
		va_start (argptr, fmt);
		vprintf (fmt, argptr);
		va_end (argptr);
	}
}

void
log_info (char* fmt, ...)
{
	va_list argptr;
	va_start (argptr, fmt);
	vprintf (fmt, argptr);
	va_end (argptr);
}
