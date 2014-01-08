/* karuiwm layout definitions */

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

/* used layouts */
static Layout layouts[] = {
	{ icon_rstack, rstack },
	{ icon_bstack, bstack },
	{ NULL,        NULL },
};

