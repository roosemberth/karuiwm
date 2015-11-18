#ifndef _KARUIBAR_BUTTON_BINDING_H
#define _KARUIBAR_BUTTON_BINDING_H

#include "action.h"
#include "argument.h"

struct button_binding {
	struct button_binding *prev, *next;
	int unsigned mod;
	int unsigned button;
	struct action *action;
	union argument arg;
};

#endif /* ndef _KARUIBAR_BUTTON_BINDING_H */
