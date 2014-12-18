#include "rstack.h"

void
rstack(struct client **clients, size_t nc, size_t nmaster, float mfact,
       int unsigned sw, int unsigned sh)
{
	int unsigned i, w, h;
	int x;

	/* draw master area */
	x = 0;
	w = nmaster == nc ? sw : (int unsigned) (mfact * (float) sw);
	h = (int unsigned) (sh / nmaster);
	DEBUG("rstack: w=%u, h=%u", w, h);
	for (i = 0; i < nmaster; ++i)
		moveresizeclient(clients[i], x, (int) (i*h),
		                 w - 2*BORDERWIDTH, h - 2*BORDERWIDTH);
	if (nmaster == nc)
		return;

	/* draw stack area */
	x = i > 0 ? (int) (mfact * (float) sw) : 0;
	w = i > 0 ? sw - (int unsigned) x : sw;
	h = sh / (int unsigned) (nc - nmaster);
	for (; i < nc; ++i)
		moveresizeclient(clients[i], x, (int) ((i - nmaster)*h),
		                 w - 2*BORDERWIDTH, h - 2*BORDERWIDTH);
}
