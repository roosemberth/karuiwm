#ifndef _KARUIWM_LAYOUT_H
#define _KARUIWM_LAYOUT_H

#include "client.h"
#include <stdlib.h>

#define LAYOUT_NAMELEN 512

typedef void (*layout_func)(struct client *, size_t, size_t, float,
                            int, int, int unsigned, int unsigned);

struct layout {
	struct layout *prev, *next;
	layout_func apply;
	char name[LAYOUT_NAMELEN];
};

void layout_init(void);
void layout_term(void);

struct layout *layouts;

#endif /* ndef _KARUIWM_LAYOUT_H */
