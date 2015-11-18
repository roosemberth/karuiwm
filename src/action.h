#ifndef _KARUIWM_ACTION_H
#define _KARUIWM_ACTION_H

#include "argument.h"

struct action {
	struct action *prev, *next;
	char const *name;
	void (*action)(union argument *arg);
};

#endif /* ndef _KARUIWM_ACTION_H */
