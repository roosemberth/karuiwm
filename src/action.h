#ifndef _KARUIWM_ACTION_H
#define _KARUIWM_ACTION_H

#include "argument.h"

struct action {
	struct action *prev, *next;
	char *name;
	void (*function)(union argument *arg);
};

void action_delete(struct action *a);
struct action *action_new(char const *name, void (*function)(union argument *));

#endif /* ndef _KARUIWM_ACTION_H */
