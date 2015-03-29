#include "monocle.h"

void
monocle(struct client *clients, size_t nc, size_t nmaster, float mfact,
        int sx, int sy, int unsigned sw, int unsigned sh)
{
	int unsigned i;
	struct client *c;
	(void) nmaster;
	(void) mfact;

	for (i = 0, c = clients; i < nc; ++i, c = c->next)
		client_moveresize(c, sx, sy,
		                  sw - 2*c->border, sh - 2*c->border);
}
