#ifndef _KARUIBAR_BUTTONBIND_H
#define _KARUIBAR_BUTTONBIND_H

#include "action.h"
#include "argument.h"

struct buttonbind {
	struct buttonbind *prev, *next;
	int unsigned mod;
	int unsigned button;
	struct action *action;
	union argument arg;
};

void buttonbind_delete(struct buttonbind *bb);
struct buttonbind *buttonbind_new(int unsigned mod, int unsigned button,
                                  struct action *a, union argument arg);

#endif /* ndef _KARUIBAR_BUTTONBIND_H */
