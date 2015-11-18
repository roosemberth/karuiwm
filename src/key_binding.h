#ifndef _KARUIBAR_KEY_BINDING_H
#define _KARUIBAR_KEY_BINDING_H

#include "action.h"
#include "argument.h"
#include <X11/Xlib.h>

struct key_binding {
	struct key_binding *prev, *next;
	int unsigned mod;
	KeySym key;
	struct action *action;
	union argument arg;
};

#endif /* ndef _KARUIBAR_KEY_BINDING_H */
