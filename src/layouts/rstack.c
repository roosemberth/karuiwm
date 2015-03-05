#include "rstack.h"

void
rstack(struct client *clients, size_t nc, size_t nmaster, float mfact,
       int sx, int sy, int unsigned sw, int unsigned sh)
{
	int unsigned i = 0, w, h;
	struct client *c = clients;
	int x, y;

	/* master area */
	if (nmaster > 0) {
		x = 0;
		w = nmaster == nc ? sw : (int unsigned) (mfact * (float) sw);
		h = sh / (int unsigned) nmaster;
		for (; i < nmaster; ++i, c = c->next) {
			y = (int signed) (i*h);
			client_moveresize(c, sx + x, sy + y,
			                  w - 2*c->border, h - 2*c->border);
		}
	}

	/* stack area */
	if (nc > nmaster) {
		x = i > 0 ? (int) (mfact * (float) sw) : 0;
		w = i > 0 ? sw - (int unsigned) x : sw;
		h = sh / (int unsigned) (nc - nmaster);
		for (; i < nc; ++i, c = c->next) {
			y = (int signed) ((i - nmaster)*h);
			client_moveresize(c, sx + x, sy + y,
			                  w - 2*c->border, h - 2*c->border);
		}
	}
}
