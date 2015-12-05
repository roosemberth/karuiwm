#include "config.h"
#include "xresource.h"
#include "list.h"
#include "karuiwm.h"
#include "util.h"
#include "string.h"
#include "strings.h"
#include "input.h"

#define CONFIG_BUFSIZE 256

int
config_get_bool(char const *key, bool def, bool *ret)
{
	char str[CONFIG_BUFSIZE];

	if (config_get_string(key, NULL, str, CONFIG_BUFSIZE) < 0) {
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
	char str[CONFIG_BUFSIZE];

	if (config_get_string(key, NULL, str, CONFIG_BUFSIZE) < 0) {
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
	char str[CONFIG_BUFSIZE];

	if (config_get_string(key, NULL, str, CONFIG_BUFSIZE) < 0) {
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
	char str[CONFIG_BUFSIZE];

	if (config_get_string(key, NULL, str, CONFIG_BUFSIZE) < 0) {
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
config_get_string(char const *key, char const *def, char *ret, size_t retlen)
{
	int unsigned i;
	struct xresource *xr;

	for (i = 0, xr = config.xresources;
	     i < config.nxresources;
	     ++i, xr = xr->next)
	{
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

	config.xresources = NULL;
	config.nxresources = 0;

	/* filter karuiwm-specific X resource entries */
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
		LIST_APPEND(&config.xresources, xr);
		++config.nxresources;
	} while ((line = strtok(NULL, "\n")) != NULL);
	sfree(xrmstr);
	sfree(prefix);

	/* set default values */
	char modstr[2];
	(void) config_get_colour("border.colour", 0x222222, &config.border.colour);
	(void) config_get_colour("border.colour_focus", 0x00FF00, &config.border.colour_focus);
	(void) config_get_int("border.width", 1, (int signed *) &config.border.width);
	(void) config_get_string("modifier", "W", modstr, 2);
	config.modifier = input_parse_modstr(modstr);

	return 0;
}

void
config_term(void)
{
	struct xresource *xr;

	while (config.nxresources > 0) {
		xr = config.xresources;
		LIST_REMOVE(&config.xresources, xr);
		--config.nxresources;
		xresource_delete(xr);
	}
}
