#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#include <stdlib.h>

void list_append(void ***l, size_t *lsize, void *e, size_t esize,
                 char const *ctx);
void list_prepend(void ***l, size_t *lsize, void *e, size_t esize,
                  char const *ctx);
signed list_remove(void ***l, size_t *lsize, void *e, size_t esize,
                char const *ctx);
void list_shift(void **l, unsigned dst, unsigned src);

/* memory allocation */
void *scalloc(size_t nmemb, size_t size, char const *ctx);
void *smalloc(size_t size, char const *context);
void *srealloc(void *ptr, size_t size, char const *ctx);
void sfree(void *ptr);

#endif /* _UTIL_H */
