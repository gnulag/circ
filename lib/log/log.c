#include "log.h"
#include "config/config.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

void
log_debug (char* fmt, ...)
{
    struct ConfigType *config = get_config();

	va_list argptr;
	va_start (argptr, fmt);
	if (config->debug) {
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
