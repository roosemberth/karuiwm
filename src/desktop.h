#ifndef _DESKTOP_H
#define _DESKTOP_H

#include "client.h"
#include <stdbool.h>

/* TODO split client list in two: tiled and floating */
struct desktop {
	size_t nt, nf, nmaster;
	struct client *tiled, *floating, *selcli;
	float mfact;
	int x, y, posx, posy;
	int unsigned w, h;
};

void desktop_arrange(struct desktop *d);
void desktop_attach_client(struct desktop *d, struct client *c);
bool desktop_contains_client(struct desktop *d, struct client *c);
void desktop_delete(struct desktop *d);
void desktop_detach_client(struct desktop *d, struct client *c);
void desktop_float_client(struct desktop *d, struct client *c, bool floating);
void desktop_fullscreen_client(struct desktop *d, struct client *c,
                               bool fullscreen);
void desktop_hide(struct desktop *d);
int desktop_locate_window(struct desktop *d, Window win, struct client **c);
struct desktop *desktop_new(int posx, int posy);
void desktop_set_clientmask(struct desktop *d, long);
void desktop_set_mfact(struct desktop *d, float mfact);
void desktop_set_nmaster(struct desktop *d, size_t nmaster);
void desktop_shift_client(struct desktop *d, int dir);
void desktop_show(struct desktop *d);
void desktop_step_focus(struct desktop *d, int dir);
void desktop_update_focus(struct desktop *d);
void desktop_update_geometry(struct desktop *d,
                             int x, int y, int unsigned w, int unsigned h);
void desktop_zoom(struct desktop *d);

#endif /* ndef _DESKTOP_H */
