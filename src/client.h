#ifndef _KARUIWM_CLIENT_H
#define _KARUIWM_CLIENT_H

#include "karuiwm.h"
#include "button_binding.h"

#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdio.h>

enum client_state { STATE_NORMAL, STATE_FULLSCREEN, STATE_SCRATCHPAD };

struct client {
	struct client *prev, *next; /* list.h */
	struct desktop *desktop;
	int x, y, floatx, floaty;
	int unsigned w, h, floatw, floath, border;
	char name[BUFSIZ];
	Window win;
	bool floating, dialog, visible;
	enum client_state state;
	int unsigned basew, baseh, incw, inch, maxw, maxh, minw, minh;
	size_t nsup;
	Atom *supported;
};

void client_delete(struct client *);
void client_grab_buttons(struct client *c, size_t nb, struct button_binding *buttons);
void client_kill(struct client *c);
void client_move(struct client *c, int x, int y);
void client_moveresize(struct client *c, int x, int y, int unsigned w, int unsigned h);
struct client *client_new(Window win);
Atom client_query_atom(struct client *c, Atom property);
void client_query_dialog(struct client *c);
void client_query_dimension(struct client *c);
void client_query_fullscreen(struct client *);
void client_query_name(struct client *c);
void client_query_sizehints(struct client *c);
void client_query_supported_atoms(struct client *c);
void client_query_transient(struct client *c);
void client_resize(struct client *c, int unsigned w, int unsigned h);
int client_send_atom(struct client *c, size_t natoms, ...);
void client_set_border(struct client *c, int unsigned border);
void client_set_dialog(struct client *c, bool dialog);
void client_set_floating(struct client *c, bool floating);
void client_set_focus(struct client *c, bool focus);
void client_set_fullscreen(struct client *c, bool fullscreen);
void client_set_visibility(struct client *c, bool visible);
bool client_supports_atom(struct client *c, Atom atom);

#endif /* ndef _KARUIWM_CLIENT_H */
