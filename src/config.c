#include "config.h"
#include "xresource.h"
#include "list.h"
#include "karuiwm.h"
#include "util.h"
#include "string.h"
#include "strings.h"
#include "keybind.h"

#define DEFAULT_LINBUFSIZE 256

static void init_default(void);

static char *linbuf;
static size_t linbufsize;
static struct xresource *xresources;
static size_t nxresources;

int
config_get_bool(char const *key, bool def, bool *ret)
{
	char str[linbufsize];

	if (config_get_string(key, NULL, str, linbufsize) < 0) {
		*ret = def;
		return -1;
	}
	*ret = strcasecmp(str, "true") == 0 || strcasecmp(str, "1") == 0;
	return 0;
}

int
config_get_colour(char const *key, uint32_t def, uint32_t *ret)
{
	XColor xcolour;
	char str[linbufsize];

	if (config_get_string(key, NULL, str, linbufsize) < 0) {
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
config_get_float(char const *key, float def, float *ret)
{
	char str[linbufsize];

	if (config_get_string(key, NULL, str, linbufsize) < 0) {
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
config_get_int(char const *key, int def, int *ret)
{
	char str[linbufsize];

	if (config_get_string(key, NULL, str, linbufsize) < 0) {
		*ret = def;
		return -1;
	}
	if (sscanf(str, "%i", ret) < 1) {
		WARN("X resources: %s: expected integer, found `%s`", key, str);
		return -1;
	}
	return 0;
}

struct keybind *
config_get_keybinds(void)
{
	struct xresource *xr;
	struct keybind *kb, *keybinds = NULL;
	int unsigned i;

	for (i = 0, xr = xresources; i < nxresources; ++i, xr = xr->next) {
		if (strncmp(xr->key, "keysym.", 7) != 0)
			continue;
		kb = keybind_fromstring(xr->key + 7, xr->value);
		if (kb == NULL) {
			WARN("failed to create key binding for `%s: %s`",
			     xr->key, xr->value);
			continue;
		}
		LIST_APPEND(&keybinds, kb);
	}

	return keybinds;
}

int
config_get_string(char const *key, char const *def, char *ret, size_t retlen)
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
	if (def != NULL)
		strncpy(ret, def, retlen);
	ret[retlen - 1] = '\0';
	return -1;
}

int
config_init(void)
{
	char *xrmstr, *line, *prefix;
	struct xresource *xr;

	/* initialise buffer */
	linbufsize = 0;
	linbuf = NULL;
	config_set_bufsize(DEFAULT_LINBUFSIZE);

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
	prefix = strdupf("%s.", karuiwm.env.APPNAME);
	line = strtok(xrmstr, "\n");
	do {
		xr = xresource_new(prefix, line);
		if (xr == NULL)
			continue;
		LIST_APPEND(&xresources, xr);
		++nxresources;
	} while ((line = strtok(NULL, "\n")) != NULL);
	sfree(xrmstr);
	sfree(prefix);

	init_default();

	return 0;
}

void
config_set_bufsize(size_t size)
{
	linbufsize = size;
	linbuf = srealloc(linbuf, linbufsize, "configuration line buffer");
	if (linbufsize == 0)
		linbuf = NULL;
}

void
config_term(void)
{
	struct xresource *xr;

	config_set_bufsize(0);
	while (nxresources > 0) {
		xr = xresources;
		LIST_REMOVE(&xresources, xr);
		--nxresources;
		xresource_delete(xr);
	}
}

static void
init_default(void)
{
	char modstr[2];
	(void) config_get_colour("border.colour", 0x222222, &config.border.colour);
	(void) config_get_colour("border.colour_focus", 0x00FF00, &config.border.colour_focus);
	(void) config_get_int("border.width", 1, (int signed *) &config.border.width);
	(void) config_get_string("modifier", "W", modstr, 2);
	config.modifier = key_mod_fromstring(modstr);
}
