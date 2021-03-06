#ifndef _KARUIWM_UTIL_H
#define _KARUIWM_UTIL_H

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>

#define DEBUG(...)   print(stderr,LOG_DEBUG,  __FILE__,__LINE__,__VA_ARGS__)
#define EVENT(...)   print(stderr,LOG_EVENT,  __FILE__,__LINE__,__VA_ARGS__)
#define VERBOSE(...) print(stdout,LOG_VERBOSE,__FILE__,__LINE__,__VA_ARGS__)
#define NORMAL(...)  print(stdout,LOG_NORMAL, __FILE__,__LINE__,__VA_ARGS__)
#define NOTICE(...)  print(stdout,LOG_NOTICE, __FILE__,__LINE__,__VA_ARGS__)
#define WARN(...)    print(stderr,LOG_WARN,   __FILE__,__LINE__,__VA_ARGS__)
#define ERROR(...)   print(stderr,LOG_ERROR,  __FILE__,__LINE__,__VA_ARGS__)
#define FATAL(...) { print(stderr,LOG_FATAL,  __FILE__,__LINE__,__VA_ARGS__); \
                     exit(EXIT_FAILURE); }

/* enumerations */
enum log_level { LOG_FATAL, LOG_ERROR, LOG_WARN, LOG_NOTICE, LOG_NORMAL,
                 LOG_VERBOSE, LOG_EVENT, LOG_DEBUG };

/* output */
void print(FILE *f, enum log_level level, char const *filename,
           int unsigned line, char const *format, ...)
           __attribute__((format(printf,5,6)));
void set_log_level(enum log_level level);

/* memory allocation */
void *scalloc(size_t nmemb, size_t size, char const *ctx);
void *smalloc(size_t size, char const *context);
void *srealloc(void *ptr, size_t size, char const *ctx);
void sfree(void *ptr);

/* strings */
char *strdupf(char const *format, ...);
int vstrlenf(char const *format, va_list ap);

#endif /* ndef _KARUIWM_UTIL_H */
