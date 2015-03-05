#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>

#define DEBUG(...)   print(stdout,LOG_DEBUG,  __FILE__,__LINE__,__VA_ARGS__)
#define EVENT(...)   print(stderr,LOG_EVENT,  __FILE__,__LINE__,__VA_ARGS__)
#define VERBOSE(...) print(stdout,LOG_VERBOSE,__FILE__,__LINE__,__VA_ARGS__)
#define NORMAL(...)  print(stdout,LOG_NORMAL, __FILE__,__LINE__,__VA_ARGS__)
#define NOTICE(...)  print(stdout,LOG_NOTICE, __FILE__,__LINE__,__VA_ARGS__)
#define WARN(...)    print(stderr,LOG_WARN,   __FILE__,__LINE__,__VA_ARGS__)
#define ERROR(...)   print(stderr,LOG_ERROR,  __FILE__,__LINE__,__VA_ARGS__)
#define DIE(...)    {print(stderr,LOG_FATAL,  __FILE__,__LINE__,__VA_ARGS__); \
                     exit(EXIT_FAILURE);}

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

#endif /* ndef _UTIL_H */
