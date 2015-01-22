#include "util.h"
#include "karuiwm.h"
#include <stdlib.h>

/* implementation */
void
attach(void ***l, size_t *lsize, void *e, size_t esize, char const *ctx)
{
	*l = srealloc(*l, ++(*lsize)*esize, ctx);
	*(*l+*lsize-1) = e;
}

int
detach(void ***l, size_t *lsize, void *e, size_t esize, char const *ctx)
{
	int unsigned i, pos;

	for (pos = 0; pos < *lsize && *(*l+pos) != e; ++pos);
	if (pos == *lsize)
		return -1;
	for (i = pos; i < *lsize-1; ++i)
		*(*l+i) = *(*l+i+1);
	*l = srealloc(*l, --(*lsize)*esize, ctx);
	return (int) pos;
}

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
