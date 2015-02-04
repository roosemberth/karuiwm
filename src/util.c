#include "util.h"
#include "karuiwm.h"
#include <stdlib.h>

/* implementation */
void
list_append(void ***l, size_t *lsize, void *e, size_t esize, char const *ctx)
{
	void **nl;
	nl = srealloc(*l, ++(*lsize) * esize, ctx);
	DEBUG("*l = %p -> nl = %p | *lsize = %zu | e = %p", *l, nl, *lsize, e);
	*l = nl;
	(*l)[*lsize - 1] = e;
}

void
list_prepend(void ***l, size_t *lsize, void *e, size_t esize, char const *ctx)
{
	unsigned i;

	*l = srealloc(*l, ++(*lsize) * esize, ctx);
	for (i = (unsigned) *lsize - 1; i > 0; --i)
		(*l)[i] = (*l)[i - 1];
	(*l)[0] = e;
}

signed
list_remove(void ***l, size_t *lsize, void *e, size_t esize, char const *ctx)
{
	unsigned i, pos;

	for (pos = 0; pos < *lsize && *(*l + pos) != e; ++pos);
	if (pos == *lsize)
		return -1;
	for (i = pos; i < *lsize - 1; ++i)
		(*l)[i] = (*l)[i + 1];
	*l = srealloc(*l, --(*lsize) * esize, ctx);
	return (signed) pos;
}

void
list_shift(void **l, unsigned dst, unsigned src)
{
	unsigned i;
	void *e = l[src];

	if (src < dst)
		for (i = src; i < dst; ++i)
			l[i] = l[i + 1];
	else
		for (i = src; i > dst; --i)
			l[i] = l[i - 1];
	l[dst] = e;
}

void *
scalloc(size_t nmemb, size_t size, char const *context)
{
	void *ptr = calloc(nmemb, size);
	if (ptr == NULL && nmemb > 0)
		DIE("could not allocate %zu bytes for %s", nmemb * size,
		    context);
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
