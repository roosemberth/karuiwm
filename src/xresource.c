#include "xresource.h"
#include "util.h"
#include "karuiwm.h"
#include <string.h>
#include <strings.h>
#include <X11/Xlib.h>

#define BUFSIZE 256

static void xresource_get_key(struct xresource *xr, char const *key, bool host,
                              char *buf, size_t buflen);

void
xresources_delete(struct xresource *xr)
{
	XrmDestroyDatabase(xr->xdb);
	sfree(xr->name);
	sfree(xr);
}

static void
xresource_get_key(struct xresource *xr, char const *key, bool host, char *buf,
                  size_t buflen)
{
	(void) snprintf(buf, buflen, "%s%s%s.%s", xr->name, host ? "_" : "",
	                host ? karuiwm.env.HOSTNAME : "", key);
}

struct xresource *
xresource_new(char const *name)
{
	char *xrm;
	XrmDatabase xdb;
	struct xresource *xr;

	xrm = XResourceManagerString(karuiwm.dpy);
	if (xrm == NULL) {
		WARN("could not get X resources manager string");
		return NULL;
	}

	XrmInitialize();
	//XrmParseCommand(...); TODO
	xdb = XrmGetStringDatabase(xrm);
	if (xdb == NULL) {
		WARN("could not get X resources string database");
		return NULL;
	}

	xr = smalloc(sizeof(struct xresource), "X resource");
	xr->xdb = xdb;
	xr->name = strdupf(name);
	return xr;
}

int
xresource_query_boolean(struct xresource *xr, char const *key, bool def,
                         bool *ret)
{
	char str[BUFSIZE];

	if (xresource_query_string(xr, key, "", str, BUFSIZE) < 0) {
		*ret = def;
		return -1;
	}
	*ret = strcasecmp(str, "true") == 0 || strcasecmp(str, "1") == 0;
	return 0;
}

int
xresource_query_colour(struct xresource *xr, char const *key, uint32_t def,
                       uint32_t *ret)
{
	XColor xcolour;
	char str[BUFSIZE];

	if (xresource_query_string(xr, key, "", str, BUFSIZE) < 0) {
		*ret = def;
		return -1;
	}
	if (XAllocNamedColor(karuiwm.dpy, karuiwm.cm, str, &xcolour, &xcolour) == 0) {
		WARN("resource '%s' carries invalid colour value", key);
		return -1;
	}
	*ret = (uint32_t) xcolour.pixel;
	return 0;
}

int
xresource_query_integer(struct xresource *xr, char const *key, int def,
                        int *ret)
{
	char str[BUFSIZE];

	if (xresource_query_string(xr, key, "", str, BUFSIZE) < 0) {
		*ret = def;
		return -1;
	}
	*ret = (int) strtol(str, NULL, 0);
	return 0;
}

int
xresource_query_string(struct xresource *xr, char const *key, char const *def,
                       char *ret, size_t retlen)
{
	char rname[BUFSIZE] = { '\0' };
	char *type = NULL;
	XrmValue val = { 0, NULL };

	/* host-specific */
	xresource_get_key(xr, key, true, rname, BUFSIZE);
	if (XrmGetResource(xr->xdb, rname, "*", &type, &val))
		goto xresources_string_success;

	/* generic */
	xresource_get_key(xr, key, false, rname, BUFSIZE);
	if (XrmGetResource(xr->xdb, rname, "*", &type, &val))
		goto xresources_string_success;

	/* default */
	strncpy(ret, def, retlen);
	return -1;

 xresources_string_success:
	strncpy(ret, val.addr, retlen);
	return 0;
}
