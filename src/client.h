#ifndef _CLIENT_H
#define _CLIENT_H

#include "karuiwm.h"
#include <X11/Xlib.h>
#include <stdbool.h>

struct client {
	int x, y, oldx, oldy;
	int unsigned w, h, oldw, oldh;
	char name[256];
	Window win;
	bool floating, fullscreen, dialog, dirty;
	int unsigned basew, baseh, incw, inch, maxw, maxh, minw, minh;
};

struct client *client_init(Window win, bool viewable);
void client_move(struct client *c, int x, int y);
void client_updatesizehints(struct client *c);
void client_updatename(struct client *c);

#endif /* _CLIENT_H */
