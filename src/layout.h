#ifndef _LAYOUT_H
#define _LAYOUT_H

#include "client.h"
#include <stdlib.h>

typedef void (*layout_func)(struct client *, size_t, size_t, float,
                            int, int, int unsigned, int unsigned);

struct layout {
	struct layout *prev, *next;
	layout_func apply;
	char name[BUFSIZ];
};

void layout_init(void);
void layout_term(void);

struct layout *layouts;

#endif /* _LAYOUT_H */
