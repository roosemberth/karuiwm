#ifndef _KARUIWM_LAYOUT_H
#define _KARUIWM_LAYOUT_H

#include "client.h"
#include <stdlib.h>

typedef void (*layout_func)(struct client *c, size_t nc, size_t nmaster,
                            float mfact, int sx, int sy, int unsigned sw,
                            int unsigned sh);

struct layout {
	struct layout *prev, *next;
	layout_func apply;
	char *name;
};

struct layout *layout_new(char const *name, layout_func apply);
void layout_delete(struct layout *layout);

#endif /* ndef _KARUIWM_LAYOUT_H */
