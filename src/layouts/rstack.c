#include "rstack.h"

void
rstack(struct client **clients, size_t nc, size_t nmaster, float mfact,
       int unsigned sw, int unsigned sh)
{
	int unsigned i = 0, w, h;
	int x;

	/* draw master area */
	if (nmaster > 0) {
		x = 0;
		w = nmaster == nc ? sw : (int unsigned) (mfact * (float) sw);
		h = (int unsigned) (sh / nmaster);
		for (; i < nmaster; ++i)
			client_moveresize(clients[i], x, (int) (i*h),
			                  w - 2*BORDERWIDTH, h - 2*BORDERWIDTH);
	}

	/* draw stack area */
	if (nc > nmaster) {
		x = i > 0 ? (int) (mfact * (float) sw) : 0;
		w = i > 0 ? sw - (int unsigned) x : sw;
		h = sh / (int unsigned) (nc - nmaster);
		for (; i < nc; ++i)
			client_moveresize(clients[i], x, (int) ((i-nmaster)*h),
			                  w - 2*BORDERWIDTH, h - 2*BORDERWIDTH);
	}
}
