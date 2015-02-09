#ifndef _DESKTOP_H
#define _DESKTOP_H

#include "client.h"
#include <stdbool.h>

/* TODO split client list in two: tiled and floating */
struct desktop {
	size_t nc, nmaster;
	int unsigned imaster;
	struct client **clients, *selcli;
	float mfact;
	int x, y;
	int unsigned w, h;
};

void desktop_arrange(struct desktop *d);
void desktop_attach_client(struct desktop *d, struct client *c);
void desktop_delete(struct desktop *d);
void desktop_detach_client(struct desktop *d, struct client *c);
void desktop_float_client(struct desktop *d, struct client *c, bool floating);
void desktop_fullscreen_client(struct desktop *d, struct client *c,
                               bool fullscreen);
bool desktop_locate_client(struct desktop *d, struct client *c,
                           int unsigned *pos);
bool desktop_locate_neighbour(struct desktop *d,
                              struct client *c, int unsigned cpos, int dir,
                              struct client **n, int unsigned *npos);
bool desktop_locate_window(struct desktop *d, Window win,
                           struct client **c, int unsigned *pos);
struct desktop *desktop_new(void);
void desktop_restack(struct desktop *d);
void desktop_set_clientmask(struct desktop *d, long);
void desktop_set_mfact(struct desktop *d, float mfact);
void desktop_set_nmaster(struct desktop *d, size_t nmaster);
void desktop_update_focus(struct desktop *d);
void desktop_update_geometry(struct desktop *d,
                             int x, int y, int unsigned w, int unsigned h);

#endif /* ndef _DESKTOP_H */
