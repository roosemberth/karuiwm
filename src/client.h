#ifndef _CLIENT_H
#define _CLIENT_H

#include "karuiwm.h"
#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdio.h>

struct client {
	int x, y, oldx, oldy;
	int unsigned w, h, oldw, oldh;
	char name[BUFSIZ];
	Window win;
	bool floating, fullscreen, dialog, dirty;
	int unsigned basew, baseh, incw, inch, maxw, maxh, minw, minh;
};

struct client *client_init(Window win, bool viewable);
void client_move(struct client *c, int x, int y);
void client_moveresize(struct client *c, int x, int y, int unsigned w,
                       int unsigned h);
void client_refreshsizehints(struct client *c);
void client_refreshname(struct client *c);
void client_resize(struct client *c, int unsigned w, int unsigned h);
void client_setfloating(struct client *c, bool floating);
void client_term(struct client *);

#endif /* _CLIENT_H */
