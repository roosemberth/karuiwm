#ifndef _KARUIBAR_KEYBIND_H
#define _KARUIBAR_KEYBIND_H

#include "action.h"
#include "argument.h"
#include <X11/Xlib.h>

struct keybind {
	struct keybind *prev, *next;
	int unsigned mod;
	KeySym key;
	struct action *action;
	union argument arg;
};

void keybind_delete(struct keybind *kb);
struct keybind *keybind_new(int unsigned mod, KeySym key, struct action *action,
                            union argument arg);

#endif /* ndef _KARUIBAR_KEYBIND_H */
