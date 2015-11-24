#include "keybind.h"
#include "util.h"

void
keybind_delete(struct keybind *kb)
{
	sfree(kb);
}

struct keybind *
keybind_new(int unsigned mod, KeySym key, struct action *a, union argument arg)
{
	struct keybind *kb;

	kb = smalloc(sizeof(struct keybind), "key binding");
	kb->mod = mod;
	kb->key = key;
	kb->action = a;
	kb->arg = arg;
	return kb;
}

struct keybind *
keybind_new_fromstring(char const *keystr, struct action *a, union argument arg)
{
	/* TODO */
	(void) keystr;
	(void) a;
	(void) arg;
	return NULL;
}
