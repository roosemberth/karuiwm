#include "util.h"
#include "karuiwm.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>

/* variables */
static enum log_level log_level;

/* implementation */
void
list_append(void ***l, size_t *lsize, void *e, size_t esize, char const *ctx)
{
	void **nl;
	nl = srealloc(*l, ++(*lsize) * esize, ctx);
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

void
print(FILE *f, enum log_level level, char const *filename, unsigned line,
      char const *format, ...)
{
	va_list args;
	time_t rawtime;
	struct tm *date;
	char const *col;

	if (level > log_level)
		return;

	/* application name & timestamp */
	rawtime = time(NULL);
	date = localtime(&rawtime);
	(void) fprintf(f, APPNAME" [%04d-%02d-%02d %02d:%02d:%02d] ",
	               date->tm_year+1900, date->tm_mon, date->tm_mday,
	               date->tm_hour, date->tm_min, date->tm_sec);

	/* log level */
	switch (level) {
	case LOG_FATAL : col = "\033[31mFATAL\033[0m "; break;
	case LOG_ERROR : col = "\033[31mERROR\033[0m "; break;
	case LOG_WARN  : col = "\033[33mWARN\033[0m " ; break;
	case LOG_NOTICE: col = "\033[1mNOTICE\033[0m "; break;
	case LOG_EVENT : col = "\033[35mEVENT\033[0m "; break;
	case LOG_DEBUG : col = "\033[34mDEBUG\033[0m "; break;
	default        : col = "";
	}
	(void) fprintf(f, "%s", col);

	/* position */
	if (level >= LOG_EVENT)
		(void) fprintf(f, "\033[32m%s:%u\033[0m ", filename, line);

	/* message */
	va_start(args, format);
	(void) vfprintf(f, format, args);
	va_end(args);

	(void) fprintf(f, "\n");
	(void) fflush(f);
}

void
set_log_level(enum log_level level)
{
	log_level = level;
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
