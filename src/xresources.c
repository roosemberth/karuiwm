#define _POSIX_SOURCE

#include "xresources.h"
#include "util.h"
#include "karuiwm.h"
#include "list.h"
#include <string.h>
#include <strings.h>
#include <X11/Xlib.h>
#include <errno.h>

#define DEFAULT_BUFSIZE 256

struct xresource {
	struct xresource *prev, *next;
	char *key;
	char *value;
};

static void delete(struct xresource *xr);
static struct xresource *new(char const *line);

static char *linbuf = NULL;
static size_t linbufsize;
static struct xresource *xresources;
static size_t nxresources;

static void
delete(struct xresource *xr)
{
	sfree(xr->key);
	sfree(xr->value);
	sfree(xr);
}

static struct xresource *
new(char const *line)
{
	struct xresource *xr;
	int unsigned offset, length;
	char *key, *value;

	/* key */
	for (offset = 0; line[offset] == ' ' || line[offset] == '\t'; ++offset);
	for (length = 0; line[offset + length] != ' '
	              && line[offset + length] != '\t'
	              && line[offset + length] != ':'; ++length);
	if (length == 0) {
		WARN("%s: empty key", line);
		return NULL;
	}
	key = smalloc(length + 1, "X resource key");
	strncpy(key, line + offset, length);
	key[length] = '\0';

	/* value */
	for (offset = 0; line[offset] != ':'; ++offset);
	++offset;
	for (; line[offset] == ' ' || line[offset] == '\t'; ++offset);
	length = (int unsigned) strlen(line) - offset;
	for (; (line[offset+length-1] == ' ' || line[offset+length-1] == '\t')
	     && length > 0; --length);
	if (length == 0) {
		WARN("%s: empty value", line);
		sfree(key);
		return NULL;
	}
	value = smalloc(length + 1, "X resource value");
	strncpy(value, line + offset, length);
	value[length] = '\0';

	/* X resource */
	xr = smalloc(sizeof(struct xresource), "X resource");
	xr->key = key;
	xr->value = value;

	return xr;
}

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

int
xresources_init(char const *appname)
{
	char *xrmstr;
	char const *line;
	size_t appnamelen;
	struct xresource *xr;

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
			xr = new(line);
			if (xr == NULL) {
				WARN("invalid line: %s", line);
			} else {
				++nxresources;
				LIST_APPEND(&xresources, xr);
			}
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

int
xresources_string(char const *key, char const *def, char *ret, size_t retlen)
{
	int unsigned i;
	struct xresource *xr;

	for (i = 0, xr = xresources; i < nxresources; ++i, xr = xr->next) {
		if (strcmp(xr->key, key) == 0) {
			strncpy(ret, xr->value, retlen);
			ret[retlen - 1] = '\0';
			return 0;
		}
	}
	strncpy(ret, def, retlen);
	ret[retlen - 1] = '\0';
	return -1;
}

void
xresources_term(void)
{
	struct xresource *xr;

	xresources_set_bufsize(0);
	while (nxresources > 0) {
		xr = xresources;
		LIST_REMOVE(&xresources, xr);
		--nxresources;
		delete(xr);
	}
}
