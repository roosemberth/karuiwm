#include "xresources.h"
#include "util.h"
#include "karuiwm.h"
#include <string.h>
#include <strings.h>
#include <X11/Xlib.h>
#include <errno.h>

#define BUFSIZE 256

static void xresources_get_key(char const *key, bool host, char *buf,
                               size_t buflen);

static XrmDatabase xdb;
static char *appname;

int
xresources_boolean(char const *key, bool def, bool *ret)
{
	char str[BUFSIZE];

	if (xresources_string(key, NULL, str, BUFSIZE) < 0) {
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
	char str[BUFSIZE];

	if (xresources_string(key, NULL, str, BUFSIZE) < 0) {
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
	char str[BUFSIZE];

	if (xresources_string(key, NULL, str, BUFSIZE) < 0) {
		*ret = def;
		return -1;
	}
	if (sscanf(str, "%f", ret) < 1) {
		WARN("X resources: %s: expected float, found `%s`", key, str);
		return -1;
	}
	return 0;
}

static void
xresources_get_key(char const *key, bool host, char *buf, size_t buflen)
{
	(void) snprintf(buf, buflen, "%s%s%s.%s", appname, host ? "_" : "",
	                host ? karuiwm.env.HOSTNAME : "", key);
}

int
xresources_init(char const *name)
{
	char *xrm;

	xrm = XResourceManagerString(karuiwm.dpy);
	if (xrm == NULL) {
		WARN("could not get X resources manager string");
		return -1;
	}

	XrmInitialize();
	//XrmParseCommand(...); TODO
	xdb = XrmGetStringDatabase(xrm);
	if (xdb == NULL) {
		WARN("could not get X resources string database");
		return -1;
	}

	appname = strdupf(name);
	return 0;
}

int
xresources_integer(char const *key, int def, int *ret)
{
	char str[BUFSIZE];

	if (xresources_string(key, NULL, str, BUFSIZE) < 0) {
		*ret = def;
		return -1;
	}
	if (sscanf(str, "%i", ret) < 1) {
		WARN("X resources: %s: expected integer, found `%s`", key, str);
		return -1;
	}
	return 0;
}

int
xresources_string(char const *key, char const *def, char *ret, size_t retlen)
{
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
}

void
xresources_term(void)
{
	XrmDestroyDatabase(xdb);
	sfree(appname);
}
