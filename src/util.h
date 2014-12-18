#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#include <stdlib.h>

/* data structure */
void attach(void ***l, size_t *lsize, void *e, size_t esize, char const *ctx);
int detach(void ***l, size_t *lsize, void *e, size_t esize, char const *ctx);

/* memory allocation */
void *scalloc(size_t nmemb, size_t size, char const *ctx);
void *smalloc(size_t size, char const *context);
void *srealloc(void *ptr, size_t size, char const *ctx);
void sfree(void *ptr);

#endif /* _UTIL_H */
