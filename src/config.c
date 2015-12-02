#include "config.h"
#include "xresource.h"
#include "list.h"
#include "karuiwm.h"
#include "util.h"
#include "string.h"
#include "strings.h"
#include "keybind.h"
#include "buttonbind.h"

#define CONFIG_BUFSIZE 256

static int extract_action_argument(struct action **action, union argument *arg,
                                   char const *actionargstr);
static int unsigned extract_mod(char const *modstr);
static int extract_mod_sym(int unsigned *mod, int unsigned *buttonsym,
                           KeySym *keysym, bool button, char const *modsymstr);
static void init_default(void);
static void scan_binds(void);

static struct xresource *xresources;
static size_t nxresources;

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
	scan_binds();

	return 0;
}

void
config_term(void)
{
	struct xresource *xr;

	while (nxresources > 0) {
		xr = xresources;
		LIST_REMOVE(&xresources, xr);
		--nxresources;
		xresource_delete(xr);
	}
}

static int
extract_action_argument(struct action **action, union argument *arg,
                        char const *actionargstr)
{
	int unsigned i;
	size_t actionargstrlen = strlen(actionargstr);
	char *actionstr, *argstr;
	int retval = -1;

	/* split action:argument */
	for (i = 0; i < actionargstrlen && actionargstr[i] != ':'; ++i);
	actionstr = strdupf("%s", actionargstr);
	actionstr[i] = '\0';
	argstr = (i == actionargstrlen) ? actionstr + i : actionstr + i + 1;

	/* action */
	for (i = 0, *action = actions;
	     i < nactions && strcmp((*action)->name, actionstr) != 0;
	     ++i, *action = (*action)->next);
	if (i == nactions) {
		WARN("action not found: `%s`", actionstr);
		goto extract_action_argument_out;
	}

	/* argument */
	if (argument_fromstring(arg, argstr, (*action)->argtype) < 0) {
		WARN("action `%s` requires argument of type %s, but `%s` given",
		     (*action)->name,
		     argument_typestring((*action)->argtype), argstr);
		goto extract_action_argument_out;
	}

	retval = 0;
 extract_action_argument_out:
	sfree(actionstr);
	return retval;
}

static int unsigned
extract_mod(char const *modstr)
{
	switch (modstr[0]) {
	case 'M': return config.modifier;
	case 'S': return ShiftMask;
	case 'C': return ControlMask;
	case 'W': return Mod4Mask;
	case 'A': return Mod1Mask;
	default: WARN("unknown modifier: `%s`", modstr);
	}
	return 0;
}

static int
extract_mod_sym(int unsigned *mod, int unsigned *button, KeySym *key,
                bool isbutton, char const *modsymstr)
{
	int unsigned sympos;
	size_t modsymstrlen = strlen(modsymstr);
	char *modstr;
	char const *modtok;

	/* symbol */
	for (sympos = (int unsigned) modsymstrlen;
	     sympos > 0 && modsymstr[sympos - 1] != '-';
	     --sympos);
	if (isbutton) {
		/* button */
		*button = (int unsigned) (modsymstr[sympos] - 48);
		if (*button < 1 || *button > 5) {
			WARN("button not found: `%s`", modsymstr + sympos);
			return -1;
		}
	} else {
		/* key */
		*key = XStringToKeysym(modsymstr + sympos);
		if (*key == NoSymbol) {
			WARN("key symbol not found: `%s`", modsymstr+sympos);
			return -1;
		}
	}

	/* modifier */
	*mod = 0;
	modstr = strdupf("%s", modsymstr);
	modtok = strtok(modstr, "-");
	do {
		if (modtok == modstr + sympos)
			break;
		*mod |= extract_mod(modtok);
	} while ((modtok = strtok(NULL, "-")) != NULL);
	sfree(modstr);

	return 0;
}

static void
init_default(void)
{
	char modstr[2];
	(void) config_get_colour("border.colour", 0x222222, &config.border.colour);
	(void) config_get_colour("border.colour_focus", 0x00FF00, &config.border.colour_focus);
	(void) config_get_int("border.width", 1, (int signed *) &config.border.width);
	(void) config_get_string("modifier", "W", modstr, 2);
	config.modifier = extract_mod(modstr);
}

static void
scan_binds(void)
{
	/* FIXME magic numbers! pragmas! */

	struct xresource *xr;
	struct buttonbind *bb;
	struct keybind *kb;
	int unsigned i;
	int unsigned mod;
	int unsigned button;
	KeySym key;
	struct action *action;
	union argument arg;
	bool isbutton, iskey;

	config.nbuttonbinds = config.nkeybinds = 0;
	config.buttonbinds = NULL;
	config.keybinds = NULL;

	for (i = 0, xr = xresources; i < nxresources; ++i, xr = xr->next) {
		/* determine if it's a button/key bind */
		isbutton = strncmp(xr->key, "button.", 7) == 0;
		iskey = strncmp(xr->key, "keysym.", 7) == 0;
		if (!isbutton && !iskey)
			continue;

		/* parse */
		if (extract_mod_sym(&mod, &button, &key, isbutton, xr->key + 7) < 0
		|| extract_action_argument(&action, &arg, xr->value) < 0) {
			WARN("failed to extract %s binding from `%s: %s`",
			     isbutton ? "button" : "key", xr->key, xr->value);
			continue;
		}

		/* add to list */
		if (isbutton) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" /* for button */
			bb = buttonbind_new(mod, button, action, arg);
#pragma GCC diagnostic pop
			LIST_APPEND(&config.buttonbinds, bb);
			++config.nbuttonbinds;
		} else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" /* for key */
			kb = keybind_new(mod, key, action, arg);
#pragma GCC diagnostic pop
			LIST_APPEND(&config.keybinds, kb);
			++config.nkeybinds;
		}
	}
}
