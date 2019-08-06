#ifndef LOG_H
#define LOG_H

#ifndef DEBUG
#define DEBUG 0
#endif

void
log_info (char *fmt, ...);

void
log_debug (char *fmt, ...);

#endif /* LOG_H */
