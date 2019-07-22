#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
log_debug (char* fmt, ...)
{
	va_list argptr;
	va_start (argptr, fmt);
	if (getenv ("CIRC_DEBUG")) {
		vprintf (fmt, argptr);
	}
	va_end (argptr);
}

void
log_info (char* fmt, ...)
{
	va_list argptr;
	va_start (argptr, fmt);
	vprintf (fmt, argptr);
	va_end (argptr);
}
