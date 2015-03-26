#include "util.h"
#include "karuiwm.h"
#include <stdarg.h>
#include <time.h>
#include <string.h>

#define ESC "\033"
#define ESC_BOLD    ESC"[1m"
#define ESC_BLACK   ESC"[30m"
#define ESC_RED     ESC"[31m"
#define ESC_GREEN   ESC"[32m"
#define ESC_YELLOW  ESC"[33m"
#define ESC_BLUE    ESC"[34m"
#define ESC_MAGENTA ESC"[35m"
#define ESC_CYAN    ESC"[36m"
#define ESC_WHITE   ESC"[37m"
#define ESC_RESET   ESC"[0m"

/* variables */
static enum log_level log_level;

/* implementation */
void
print(FILE *f, enum log_level level, char const *filename, int unsigned line,
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
	               date->tm_year+1900, date->tm_mon + 1, date->tm_mday,
	               date->tm_hour, date->tm_min, date->tm_sec);

	/* log level */
	switch (level) {
	case LOG_FATAL : col = ESC_BOLD ESC_RED    "FATAL" ESC_RESET " "; break;
	case LOG_ERROR : col =          ESC_RED    "ERROR" ESC_RESET " "; break;
	case LOG_WARN  : col =          ESC_YELLOW "WARN"  ESC_RESET " "; break;
	case LOG_NOTICE: col =          ESC_CYAN   "NOTICE"ESC_RESET " "; break;
	case LOG_EVENT : col =          ESC_MAGENTA"EVENT" ESC_RESET " "; break;
	case LOG_DEBUG : col =          ESC_BLUE   "DEBUG" ESC_RESET " "; break;
	default        : col =                                        "";
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
		FATAL("could not allocate %zu bytes for %s", nmemb * size,
		      context);
	return ptr;
}

void *
smalloc(size_t size, char const *context)
{
	void *ptr = malloc(size);
	if (ptr == NULL)
		FATAL("could not allocate %zu bytes for %s", size, context);
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
		FATAL("could not allocate %zu bytes for %s", size, context);
	return ptr;
}

void
sfree(void *ptr)
{
	free(ptr);
}
