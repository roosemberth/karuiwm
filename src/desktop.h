#ifndef _KARUIWM_DESKTOP_H
#define _KARUIWM_DESKTOP_H

#include "client.h"
#include "workspace.h"
#include "monitor.h"
#include "argument.h"
#include <stdbool.h>

struct desktop {
	struct desktop *prev, *next; /* list.h */
	struct workspace *workspace;
	struct monitor *monitor;
	size_t nt, nf, nmaster;
	struct client *tiled, *floating, *selcli;
	struct workspace *ws;
	struct layout *sellayout;
	float mfact;
	int posx, posy;
	bool focus;
};

void desktop_arrange(struct desktop *d);
void desktop_attach_client(struct desktop *d, struct client *c);
void desktop_delete(struct desktop *d);
void desktop_detach_client(struct desktop *d, struct client *c);
void desktop_float_client(struct desktop *d, struct client *c, bool floating);
void desktop_focus_client(struct desktop *d, struct client *c);
void desktop_fullscreen_client(struct desktop *d, struct client *c,
                               bool fullscreen);
void desktop_kill_client(struct desktop *d);
int desktop_locate_window(struct desktop *d, struct client **c, Window win);
struct desktop *desktop_new(void);
void desktop_set_clientmask(struct desktop *d, long);
void desktop_set_focus(struct desktop *d, bool focus);
void desktop_set_mfact(struct desktop *d, float mfact);
void desktop_set_nmaster(struct desktop *d, size_t nmaster);
void desktop_shift_client(struct desktop *d, int dir);
void desktop_show(struct desktop *d, struct monitor *m);
void desktop_step_client(struct desktop *d, enum list_direction dir);
void desktop_step_layout(struct desktop *d, enum list_direction dir);
void desktop_update_focus(struct desktop *d);
void desktop_zoom(struct desktop *d);

#endif /* ndef _KARUIWM_DESKTOP_H */
