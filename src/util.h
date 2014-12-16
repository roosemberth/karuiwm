#ifndef _UTIL_H
#define _UTIL_H

#include <stdint.h>
#include <stdlib.h>

void *scalloc(size_t nmemb, size_t size, char const *context);
void *smalloc(size_t size, char const *context);
void *srealloc(void *ptr, size_t size, char const *context);
void sfree(void *ptr);

#endif /* _UTIL_H */
