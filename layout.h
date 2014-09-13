/* karuiwm layout definitions */

#define LFIXED_AREA 522+2 /* width of fixed size area in the lfixed layout */
#define CHAT_AREA 260     /* width of fixed size area in the chat layout */

/* horizontally arranged master at top, horizontally arranged stack at bottom */
void
bstack(Monitor *mon)
{
	Client **tiled;
	unsigned int i, ncm, nct, w, h, bw;
	int y;

	/* get tiled clients */
	if (!mon->selws->nc || !(nct = gettiled(&tiled, mon))) {
		return;
	}
	bw = tiled[0]->border;

	/* draw master area */
	ncm = MIN(mon->selws->nmaster, nct);
	if (ncm) {
		y = mon->wy;
		w = mon->ww/ncm;
		h = ncm == nct ? mon->wh : mon->selws->mfact*mon->wh;
		for (i = 0; i < ncm; i++) {
			moveresizeclient(mon, tiled[i], mon->wx+i*w, y,
					w-2*bw, h-2*bw);
		}
	}
	if (ncm == nct) {
		free(tiled);
		return;
	}

	/* draw stack area */
	y = mon->wy+(ncm ? mon->selws->mfact*mon->wh : 0);
	w = mon->ww/(nct-ncm);
	h = ncm ? mon->h-y : mon->wh;
	for (i = ncm; i < nct; i++) {
		moveresizeclient(mon, tiled[i], mon->wx+(i-ncm)*w, y,
				w-2*bw, h-2*bw);
	}
	free(tiled);
}
static long const icon_bstack[] = {
	17, 15,
	0x00000,
	0x00000,
	0x1FFFF,
	0x1FFFF,
	0x1FFFF,
	0x1FFFF,
	0x1FFFF,
	0x00000,
	0x1F7DF,
	0x1F7DF,
	0x1F7DF,
	0x1F7DF,
	0x1F7DF,
	0x00000,
	0x00000,
};

/* vertically arranged master at left, vertically arranged stack at right */
void
rstack(Monitor *mon)
{
	Client **tiled;
	unsigned int i, ncm, nct, w, h, bw;
	int x;

	/* get tiled clients */
	if (!mon->selws->nc || !(nct = gettiled(&tiled, mon))) {
		return;
	}
	bw = tiled[0]->border;

	/* draw master area */
	ncm = MIN(mon->selws->nmaster, nct);
	if (ncm) {
		x = mon->wx;
		w = ncm == nct ? mon->ww : mon->selws->mfact*mon->ww;
		h = mon->wh/ncm;
		for (i = 0; i < ncm; i++) {
			moveresizeclient(mon, tiled[i], x, mon->wy+i*h,
					w-2*bw, h-2*bw);
		}
	}
	if (ncm == nct) {
		free(tiled);
		return;
	}

	/* draw stack area */
	x = mon->wx+(ncm ? mon->selws->mfact*mon->ww : 0);
	w = ncm ? mon->w-x : mon->ww;
	h = mon->wh/(nct-ncm);
	for (i = ncm; i < nct; i++) {
		moveresizeclient(mon, tiled[i], x, mon->wy+(i-ncm)*h,
				w-2*bw, h-2*bw);
	}
	free(tiled);
}
static long const icon_rstack[] = {
	17, 15,
	0x00000,
	0x00000,
	0x1FEFF,
	0x1FEFF,
	0x1FEFF,
	0x1FE00,
	0x1FEFF,
	0x1FEFF,
	0x1FEFF,
	0x1FE00,
	0x1FEFF,
	0x1FEFF,
	0x1FEFF,
	0x00000,
	0x00000,
};

/* fixed size one-client-master on the left, bstack on the right */
void
lfixed(Monitor *mon)
{
	Client **tiled, **ntiled;
	unsigned int i, ncm, nct, y, w, h, bw, wx, ww, wfix;
	Workspace *ws = mon->selws;

	/* get tiled clients */
	if (!mon->selws->nc || !(nct = gettiled(&tiled, mon))) {
		return;
	}
	bw = tiled[0]->border;

	/* draw fix client to the left */
	wfix = MIN(LFIXED_AREA+2*bw, mon->ww-10);
	w = nct > 1 ? wfix : mon->ww;
	moveresizeclient(mon, tiled[0], mon->wx, mon->wy, w-2*bw, mon->wh-2*bw);

	/* adjust for ordinary bstack to the right */
	nct--;
	ncm = MIN(ws->nmaster, nct);
	ntiled = tiled+1;
	wx = mon->wx+wfix;
	ww = mon->ww-wfix;

	/* draw master area */
	if (ncm) {
		y = mon->wy;
		w = ww/ncm;
		h = ncm == nct ? mon->wh : ws->mfact*mon->wh;
		for (i = 0; i < ncm; i++) {
			moveresizeclient(mon, ntiled[i], wx+i*w, y, w-2*bw, h-2*bw);
		}
	}
	if (ncm == nct) {
		free(tiled);
		return;
	}

	/* draw stack area */
	y = mon->wy+(ncm ? ws->mfact*mon->wh : 0);
	w = ww/(nct-ncm);
	h = ncm ? mon->h-y : mon->wh;
	for (i = ncm; i < nct; i++) {
		moveresizeclient(mon, ntiled[i], wx+(i-ncm)*w, y, w-2*bw, h-2*bw);
	}
	free(tiled);
}
static long const icon_lfixed[] = {
	17, 15,
	0x00000,
	0x00000,
	0x1FDFF,
	0x1FDFF,
	0x1FDFF,
	0x1FDFF,
	0x1FDFF,
	0x1FC00,
	0x1FDEF,
	0x1FDEF,
	0x1FDEF,
	0x1FDEF,
	0x1FDEF,
	0x00000,
	0x00000,
};

/* fixed size one-client-master (thin) on the left, bstack on the right */
void
chat(Monitor *mon)
{
	Client **tiled, **ntiled;
	unsigned int i, ncm, nct, y, w, h, bw, wx, ww, wfix;
	Workspace *ws = mon->selws;

	/* get tiled clients */
	if (!mon->selws->nc || !(nct = gettiled(&tiled, mon))) {
		return;
	}
	bw = tiled[0]->border;

	/* draw fix client to the left */
	wfix = MIN(CHAT_AREA+2*bw, mon->ww-10);
	w = nct > 1 ? wfix : mon->ww;
	moveresizeclient(mon, tiled[0], mon->wx, mon->wy, w-2*bw, mon->wh-2*bw);

	/* adjust for ordinary bstack to the right */
	nct--;
	ncm = MIN(ws->nmaster, nct);
	ntiled = tiled+1;
	wx = mon->wx+wfix;
	ww = mon->ww-wfix;

	/* draw master area */
	if (ncm) {
		y = mon->wy;
		w = ww/ncm;
		h = ncm == nct ? mon->wh : ws->mfact*mon->wh;
		for (i = 0; i < ncm; i++) {
			moveresizeclient(mon, ntiled[i], wx+i*w, y, w-2*bw, h-2*bw);
		}
	}
	if (ncm == nct) {
		free(tiled);
		return;
	}

	/* draw stack area */
	y = mon->wy+(ncm ? ws->mfact*mon->wh : 0);
	w = ww/(nct-ncm);
	h = ncm ? mon->h-y : mon->wh;
	for (i = ncm; i < nct; i++) {
		moveresizeclient(mon, ntiled[i], wx+(i-ncm)*w, y, w-2*bw, h-2*bw);
	}
	free(tiled);
}
static long const icon_chat[] = {
	17, 15,
	0x00000,
	0x00000,
	0x1DFBF,
	0x1DFBF,
	0x1DFBF,
	0x1DFBF,
	0x1DFBF,
	0x1C000,
	0x1DFBF,
	0x1DFBF,
	0x1DFBF,
	0x1DFBF,
	0x1DFBF,
	0x00000,
	0x00000,
};

/* used layouts */
static Layout layouts[] = {
	{ icon_rstack, rstack },
	{ icon_bstack, bstack },
	{ icon_lfixed, lfixed },
	{ NULL,        NULL },
	{ icon_chat,   chat },
};

