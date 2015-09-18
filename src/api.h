#ifndef _KARUIWM_API_H
#define _KARUIWM_API_H

#include "module.h"
#include <stdlib.h>

struct {
	/* modules */
	size_t npaths;
	char **paths;
	size_t nmodules;
	struct module *modules;
} api;

int api_init(void);
void api_term(void);

#endif /* ndef _KARUIWM_API_H */
