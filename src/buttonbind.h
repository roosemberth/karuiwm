#ifndef _KARUIBAR_BUTTONBIND_H
#define _KARUIBAR_BUTTONBIND_H

#include "action.h"
#include "argument.h"
#include "button.h"

struct buttonbind {
	struct buttonbind *prev, *next;
	struct button *button;
	struct action *action;
	union argument arg;
};

void buttonbind_delete(struct buttonbind *bb);
struct buttonbind *buttonbind_new(struct button *button, struct action *a,
                                  union argument arg);
struct buttonbind *buttonbind_fromstring(char const *buttonstr,
                                         char const *actionargstr);

#endif /* ndef _KARUIBAR_BUTTONBIND_H */
