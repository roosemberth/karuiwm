#ifndef _KARUIWM_API_H
#define _KARUIWM_API_H

#include "module.h"
#include "action.h"
#include "layout.h"
#include <stdlib.h>

struct {
	/* modules */
	size_t npaths;
	char **paths;
	size_t nmodules;
	struct module *modules;

	/* actions, layouts */
	struct action *actions;
	size_t nactions;
	struct layout *layouts;
	size_t nlayouts;
} api;

void api_add_action(struct action *a);
void api_add_layout(struct layout *l);
int api_init(void);
void api_term(void);

#endif /* ndef _KARUIWM_API_H */
