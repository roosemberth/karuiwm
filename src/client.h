#ifndef _CLIENT_H
#define _CLIENT_H

#include "karuiwm.h"
#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdio.h>

struct client {
	int x, y, floatx, floaty;
	int unsigned w, h, floatw, floath, border;
	char name[BUFSIZ];
	Window win;
	bool floating, fullscreen, dialog;
	int unsigned basew, baseh, incw, inch, maxw, maxh, minw, minh;
	struct client *trans;
};

struct client *client_init(Window win);
void client_move(struct client *c, int x, int y);
void client_moveresize(struct client *c, int x, int y, int unsigned w,
                       int unsigned h);
void client_querydialog(struct client *);
void client_querydimension(struct client *c);
void client_queryfullscreen(struct client *);
void client_queryname(struct client *c);
void client_querysizehints(struct client *c);
void client_querytransient(struct client *c);
void client_resize(struct client *c, int unsigned w, int unsigned h);
void client_setborder(struct client *c, int unsigned border);
void client_setdialog(struct client *c, bool dialog);
void client_setfloating(struct client *c, bool floating);
void client_setfullscreen(struct client *c, bool fullscreen);
void client_term(struct client *);

#endif /* _CLIENT_H */
