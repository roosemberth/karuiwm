#include "karuiwm.h"
#include "keybind.h"
#include "util.h"
#include <string.h>

void
keybind_delete(struct keybind *kb)
{
	sfree(kb);
}

struct keybind *
keybind_new(struct key *key, struct action *action, union argument arg)
{
	struct keybind *kb;

	kb = smalloc(sizeof(struct keybind), "key binding");
	kb->key = key;
	kb->action = action;
	kb->arg = arg;
	return kb;
}

struct keybind *
keybind_fromstring(char const *keystr, char const *actionargstr)
{
	struct key *key;
	struct action *action;
	union argument arg;
	struct keybind *kb = NULL;
	char *actionstr, *argstr;
	int unsigned i;
	size_t actionargstrlen = strlen(actionargstr);

	/* split action:argument string */
	for (i = 0; i < actionargstrlen && actionargstr[i] != ':'; ++i);
	actionstr = strdupf("%s", actionargstr);
	actionstr[i] = '\0';
	argstr = (i == actionargstrlen) ? actionstr + i : actionstr + i + 1;

	/* search corresponding action */
	for (i = 0, action = actions;
	     i < nactions && strcmp(action->name, actionstr) != 0;
	     ++i, action = action->next);
	if (i == nactions) {
		WARN("action not found: `%s`", actionstr);
		goto keybind_fromstring_out;
	}

	/* extract argument */
	if (argstr == NULL && action->argtype != ARGTYPE_NONE) {
		WARN("action `%s` requires non-empty argument", action->name);
		goto keybind_fromstring_out;
	}
	if (argument_fromstring(&arg, argstr, action->argtype) < 0) {
		WARN("action `%s` requires argument of type %s, but `%s` given",
		     action->name, argument_typestring(action->argtype), argstr);
		goto keybind_fromstring_out;
	}

	/* extract key */
	key = key_fromstring(keystr);
	if (key == NULL) {
		WARN("invalid key sequence: `%s`", keystr);
		goto keybind_fromstring_out;
	}

	/* compose key binding */
	kb = keybind_new(key, action, arg);

 keybind_fromstring_out:
	sfree(actionstr);
	return kb;
}
