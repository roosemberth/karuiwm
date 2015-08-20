#ifndef _KARUIWM_API_H
#define _KARUIWM_API_H

#include "module.h"
#include <stdlib.h>

#define API_PATHLEN 512

struct {
	/* modules */
	size_t npaths;
	char **paths;
	struct module *modules;
} api;

int api_init(void);
void api_term(void);

#endif /* ndef _KARUIWM_API_H */
