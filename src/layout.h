#ifndef _LAYOUT_H
#define _LAYOUT_H

void bstack(struct monitor *);
void rstack(struct monitor *);
void lfixed(struct monitor *);
void chat(struct monitor *);

int long unsigned const icon_bstack[] = {
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

int long unsigned const icon_rstack[] = {
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

int long unsigned const icon_lfixed[] = {
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

int long unsigned const icon_chat[] = {
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
struct layout layouts[] = {
	{ .icon_bitfield = icon_rstack, .func = rstack },
	{ .icon_bitfield = icon_bstack, .func = bstack },
	{ .icon_bitfield = icon_lfixed, .func = lfixed },
	{ .icon_bitfield = NULL,        .func = NULL },
	{ .icon_bitfield = icon_chat,   .func = chat },
};

#endif /* _LAYOUT_H */
