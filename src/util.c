#include "util.h"
#include "karuiwm.h"
#include <stdlib.h>

void *
scalloc(size_t nmemb, size_t size, char const *context)
{
	void *ptr = calloc(nmemb, size);
	if (ptr == NULL && nmemb > 0)
		DIE("could not allocate %zu bytes for %s", nmemb*size, context);
	return ptr;
}

void *
smalloc(size_t size, char const *context)
{
	void *ptr = malloc(size);
	if (ptr == NULL)
		DIE("could not allocate %zu bytes for %s", size, context);
	return ptr;
}

void *
srealloc(void *ptr, size_t size, char const *context)
{
	if (size == 0) {
		free(ptr);
		return NULL;
	}
	ptr = realloc(ptr, size);
	if (ptr == NULL)
		DIE("could not allocate %zu bytes for %s", size, context);
	return ptr;
}

void
sfree(void *ptr)
{
	free(ptr);
}
