#ifndef _CLIENT_H
#define _CLIENT_H

#include "karuiwm.h"
#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdio.h>

enum client_state { STATE_NORMAL, STATE_FULLSCREEN, STATE_SCRATCHPAD };

struct client {
	struct client *next, *prev;
	int x, y, floatx, floaty;
	int unsigned w, h, floatw, floath, border;
	char name[BUFSIZ];
	Window win;
	bool floating, dialog;
	enum client_state state;
	int unsigned basew, baseh, incw, inch, maxw, maxh, minw, minh;
};

void client_delete(struct client *);
void client_grab_buttons(struct client *c, size_t nb, struct button *buttons);
void client_hide(struct client *c);
void client_kill(struct client *c);
void client_move(struct client *c, int x, int y);
void client_moveresize(struct client *c, int x, int y,
                       int unsigned w, int unsigned h);
struct client *client_new(Window win);
Atom client_query_atom(struct client *c, Atom property);
void client_query_dialog(struct client *c);
void client_query_dimension(struct client *c);
void client_query_fullscreen(struct client *);
void client_query_name(struct client *c);
void client_query_sizehints(struct client *c);
void client_query_transient(struct client *c);
void client_resize(struct client *c, int unsigned w, int unsigned h);
int client_send_atom(struct client *c, size_t natoms, ...);
void client_set_border(struct client *c, int unsigned border);
void client_set_dialog(struct client *c, bool dialog);
void client_set_floating(struct client *c, bool floating);
void client_set_focus(struct client *c, bool focus);
void client_set_fullscreen(struct client *c, bool fullscreen);
void client_show(struct client *c);

#endif /* _CLIENT_H */
