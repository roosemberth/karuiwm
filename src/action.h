#ifndef _KARUIWM_ACTION_H
#define _KARUIWM_ACTION_H

#include "argument.h"

struct action {
	struct action *prev, *next;
	char *name;
	void (*function)(union argument *arg);
	enum argument_type argtype;
};

void action_delete(struct action *a);
struct action *action_new(char const *name, void (*function)(union argument *),
                          enum argument_type argtype);

#endif /* ndef _KARUIWM_ACTION_H */
