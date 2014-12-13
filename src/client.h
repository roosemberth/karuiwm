#ifndef _CLIENT_H
#define _CLIENT_H

#include <X11/Xlib.h>

struct client {
	int x, y;
	int unsigned w, h;
	int oldx, oldy, oldw, oldh;
	char name[256];
	Window win;
	bool floating, fullscreen, dialog, dirty;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int border;
};

struct client *client_init(Window win, bool viewable);
void client_move(struct monitor *mon, struct client *c, int x, int y);
void client_updatesizehints(struct client *c);
void client_updatename(struct client *);

#endif /* _CLIENT_H */
