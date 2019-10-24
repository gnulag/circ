#ifndef LOG_H
#define LOG_H

#ifndef DEBUG
#define DEBUG 1
#endif

void
log_info (char *fmt, ...);

void
log_debug (char *fmt, ...);

void
log_error (char *fmt, ...);

#endif /* LOG_H */
