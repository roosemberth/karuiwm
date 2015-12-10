#include "input.h"
#include "config.h"
#include "action.h"
#include "util.h"
#include "api.h"
#include "buttonbind.h"
#include "keybind.h"
#include "list.h"
#include "karuiwm.h"
#include <string.h>

static int extract_action_argument(struct action **action, union argument *arg,
                                   char const *actionargstr);
static int extract_mod_sym(int unsigned *mod, int unsigned *buttonsym,
                           KeySym *keysym, bool button, char const *modsymstr);
static void scan_binds(void);

static struct buttonbind *buttonbinds;
static struct keybind *keybinds;
static size_t nbuttonbinds, nkeybinds;

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
	for (i = 0, *action = api.actions;
	     i < api.nactions && strcmp((*action)->name, actionstr) != 0;
	     ++i, *action = (*action)->next);
	if (i == api.nactions) {
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
		*mod |= input_parse_modstr(modtok);
	} while ((modtok = strtok(NULL, "-")) != NULL);
	sfree(modstr);

	return 0;
}

void
input_grab_buttons(struct client *c)
{
	int unsigned i;
	struct buttonbind *bb;

	XUngrabButton(karuiwm.dpy, AnyButton, AnyModifier, c->win);
	for (i = 0, bb = buttonbinds; i < nbuttonbinds; ++i, bb = bb->next)
		XGrabButton(karuiwm.dpy, bb->button, bb->mod, c->win, False,
		            BUTTONMASK, GrabModeAsync, GrabModeAsync, None,
		            None);
}

void
input_grab_keys(void)
{
	int unsigned i;
	struct keybind *kb;

	XUngrabKey(karuiwm.dpy, AnyKey, AnyModifier, karuiwm.root);
	for (i = 0, kb = keybinds; i < nkeybinds; ++i, kb = kb->next)
		XGrabKey(karuiwm.dpy, XKeysymToKeycode(karuiwm.dpy, kb->key),
		         kb->mod, karuiwm.root, True, GrabModeAsync,
		         GrabModeAsync);
}

void
input_handle_buttonpress(int unsigned mod, int unsigned button)
{
	int unsigned i;
	struct buttonbind *bb;
	Window win = karuiwm.event->xbutton.window;

	for (i = 0, bb = buttonbinds; i < nbuttonbinds; ++i, bb = bb->next) {
		if (bb->mod == mod && bb->button == button) {
			bb->action->function(&((union argument) {.v = &win}));
			break;
		}
	}
}

void
input_handle_keypress(int unsigned mod, KeySym key)
{
	int unsigned i;
	struct keybind *kb;

	for (i = 0, kb = keybinds; i < nkeybinds; ++i, kb = kb->next) {
		if (kb->mod == mod && kb->key == key) {
			kb->action->function(&kb->arg);
			break;
		}
	}
}

int
input_init(void)
{
	scan_binds();
	return 0;
}

int unsigned
input_parse_modstr(char const *modstr)
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

void
input_term(void)
{
	struct buttonbind *bb;
	struct keybind *kb;

	while (nbuttonbinds > 0) {
		bb = buttonbinds;
		LIST_REMOVE(&buttonbinds, bb);
		--nbuttonbinds;
		buttonbind_delete(bb);
	}
	while (nkeybinds > 0) {
		kb = keybinds;
		LIST_REMOVE(&keybinds, kb);
		--nkeybinds;
		keybind_delete(kb);
	}
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

	buttonbinds = NULL;
	keybinds = NULL;
	nbuttonbinds = nkeybinds = 0;

	for (i = 0, xr = config.xresources;
	     i < config.nxresources;
	     ++i, xr = xr->next)
	{
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

		/* check compatibility */
		if (iskey && action->argtype == ARGTYPE_MOUSE) {
			WARN("cannot bind keyboard to mouse action `%s`",
			     action->name);
			continue;
		}

		/* add to list */
		if (isbutton) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" /* for button */
			bb = buttonbind_new(mod, button, action, arg);
#pragma GCC diagnostic pop
			LIST_APPEND(&buttonbinds, bb);
			++nbuttonbinds;
		} else {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmaybe-uninitialized" /* for key */
			kb = keybind_new(mod, key, action, arg);
#pragma GCC diagnostic pop
			LIST_APPEND(&keybinds, kb);
			++nkeybinds;
		}
	}
}
