#define _POSIX_SOURCE

#include "xresources.h"
#include "util.h"
#include "karuiwm.h"
#include <string.h>
#include <strings.h>
#include <X11/Xlib.h>
#include <errno.h>

#define DEFAULT_BUFSIZE 256

#if 0
static void xresources_get_key(char const *key, bool host, char *buf,
                               size_t buflen);
#endif

static char *linbuf = NULL;
static size_t linbufsize;
static char **xresources;
static size_t nxresources;
static char *_prefix;

int
xresources_boolean(char const *key, bool def, bool *ret)
{
	char str[linbufsize];

	if (xresources_string(key, NULL, str, linbufsize) < 0) {
		*ret = def;
		return -1;
	}
	*ret = strcasecmp(str, "true") == 0 || strcasecmp(str, "1") == 0;
	return 0;
}

int
xresources_colour(char const *key, uint32_t def, uint32_t *ret)
{
	XColor xcolour;
	char str[linbufsize];

	if (xresources_string(key, NULL, str, linbufsize) < 0) {
		*ret = def;
		return -1;
	}
	if (!XAllocNamedColor(karuiwm.dpy,karuiwm.cm, str, &xcolour,&xcolour)) {
		WARN("X resources: %s: expected colour code, found `%s`",
		     key, str);
		return -1;
	}
	*ret = (uint32_t) xcolour.pixel;
	return 0;
}

int
xresources_floating(char const *key, float def, float *ret)
{
	char str[linbufsize];

	if (xresources_string(key, NULL, str, linbufsize) < 0) {
		*ret = def;
		return -1;
	}
	if (sscanf(str, "%f", ret) < 1) {
		WARN("X resources: %s: expected float, found `%s`", key, str);
		return -1;
	}
	return 0;
}

#if 0
static void
xresources_get_key(char const *key, bool host, char *buf, size_t buflen)
{
	/* TODO */

	(void) key;
	(void) host;
	(void) buf;
	(void) buflen;

	/*
	(void) snprintf(buf, buflen, "%s%s%s.%s", appname, host ? "_" : "",
	                host ? karuiwm.env.HOSTNAME : "", key);
	*/
}
#endif

int
xresources_init(char const *appname)
{
	char *xrmstr;
	char const *line;
	size_t appnamelen;

	/* initialise buffer */
	linbufsize = 0;
	linbuf = NULL;
	xresources_set_bufsize(DEFAULT_BUFSIZE);

	/* initialise relevant X resources */
	xresources = NULL;
	nxresources = 0;

	/* filter relevant X resource entries */
	xrmstr = XResourceManagerString(karuiwm.dpy);
	if (xrmstr == NULL) {
		WARN("could not get X resources manager string");
		return -1;
	}
	xrmstr = strdupf("%s", xrmstr);
	appnamelen = strlen(appname);
	line = strtok(xrmstr, "\n");
	do {
		if (strncmp(line, appname, appnamelen) == 0
		&& (line[appnamelen] == '.' || line[appnamelen] == '_')) {
			++nxresources;
			xresources = srealloc(xresources,
			                      nxresources * sizeof(char *),
			                      "X resources list");
			xresources[nxresources - 1] = strdupf(line);
			DEBUG("added [%s]", line);
		}
		line = strtok(NULL, "\n");
	} while (line != NULL);
	sfree(xrmstr);

	return 0;
}

int
xresources_integer(char const *key, int def, int *ret)
{
	char str[linbufsize];

	if (xresources_string(key, NULL, str, linbufsize) < 0) {
		*ret = def;
		return -1;
	}
	if (sscanf(str, "%i", ret) < 1) {
		WARN("X resources: %s: expected integer, found `%s`", key, str);
		return -1;
	}
	return 0;
}

void
xresources_set_bufsize(size_t size)
{
	linbufsize = size;
	linbuf = srealloc(linbuf, linbufsize, "X resources line buffer");
	if (linbufsize == 0)
		linbuf = NULL;
}

void
xresources_set_prefix(char const *prefix)
{
	if (_prefix != NULL) {
		sfree(_prefix);
		_prefix = NULL;
	}
	if (prefix != NULL)
		_prefix = strdupf("%s", prefix);
}

int
xresources_string(char const *key, char const *def, char *ret, size_t retlen)
{
	(void) key;
	(void) def;
	(void) ret;
	(void) retlen;

	return -1;
#if 0
	char rname[BUFSIZE] = { '\0' };
	char *type = NULL;
	XrmValue val = { 0, NULL };

	/* host-specific */
	xresources_get_key(key, true, rname, BUFSIZE);
	if (XrmGetResource(xdb, rname, "*", &type, &val))
		goto xresources_string_success;

	/* generic */
	xresources_get_key(key, false, rname, BUFSIZE);
	if (XrmGetResource(xdb, rname, "*", &type, &val))
		goto xresources_string_success;

	/* default */
	if (def != NULL)
		(void) strncpy(ret, def, retlen);
	return -1;

 xresources_string_success:
	(void) strncpy(ret, val.addr, retlen);
	return 0;
#endif
}

void
xresources_term(void)
{
	int unsigned i;

	xresources_set_bufsize(0);
	for (i = 0; i < nxresources; ++i)
		sfree(xresources[i]);
	sfree(xresources);
}
