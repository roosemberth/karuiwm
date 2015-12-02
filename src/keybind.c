#include "keybind.h"
#include "util.h"
#include <string.h>

void
keybind_delete(struct keybind *kb)
{
	if (kb->action->argtype == ARGTYPE_STRING)
		sfree(kb->arg.v);
	sfree(kb);
}

struct keybind *
keybind_new(int unsigned mod, KeySym key, struct action *action,
            union argument arg)
{
	struct keybind *kb;

	kb = smalloc(sizeof(struct keybind), "key binding");
	kb->mod = mod;
	kb->key = key;
	kb->action = action;
	kb->arg = arg;
	return kb;
}
