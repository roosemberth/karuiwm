#include "karuiwm.h"
#include "client.h"
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define BORDERWIDTH 1

struct client *
client_init(Window win, bool viewable)
{
	struct client *c;
	XWindowAttributes wa;

	/* ignore buggy windows or windows with override_redirect */
	if (!XGetWindowAttributes(kwm.dpy, win, &wa)) {
		WARN("XGetWindowAttributes() failed for window %d", win);
		return NULL;
	}
	if (wa.override_redirect) {
		return NULL;
	}

	/* ignore unviewable windows if we request for viewable windows */
	if (viewable && wa.map_state != IsViewable) {
		return NULL;
	}

	/* create client */
	c = malloc(sizeof(struct client));
	if (!c) {
		ERROR("could not allocate %zu bytes for client", sizeof(struct client));
		exit(EXIT_FAILURE);
	}
	c->win = win;
	c->floating = false;
	c->fullscreen = false;
	if (c->floating) {
		c->x = wa.x;
		c->y = wa.y;
		c->w = wa.width;
		c->h = wa.height;
	}
	c->dirty = false;
	XSetWindowBorderWidth(kwm.dpy, c->win, BORDERWIDTH);
	client_updatesizehints(c);
	c->name[0] = 0;
	client_updatename(c);
	grabbuttons(c, false);
	return c;
}

void
client_move(struct client *c, int x, int y)
{
	c->x = x;
	c->y = y;
	XMoveWindow(kwm.dpy, c->win, c->x, c->y);
}

void
client_updatename(struct client *c)
{
	XTextProperty xtp;
	int n;
	char **list;
	bool broken=false;

	XGetTextProperty(kwm.dpy, c->win, &xtp, netatom[NetWMName]);
	if (!xtp.nitems) {
		broken = true;
	} else {
		if (xtp.encoding == XA_STRING) {
			strncpy(c->name, (char const *) xtp.value, 255);
		} else {
			if (XmbTextPropertyToTextList(kwm.dpy, &xtp, &list, &n) >= Success &&
					n > 0 && list) {
				strncpy(c->name, list[0], 255);
				XFreeStringList(list);
			}
		}
	}
	if (broken || c->name[0] == 0) {
		strcpy(c->name, "[broken]");
	}
	c->name[255] = 0;
	XFree(xtp.value);

	DEBUG("window %lu has name '%s'", c->win, c->name);
}

void
client_updatesizehints(struct client *c)
{
	long size;
	XSizeHints hints;

	if (!XGetWMNormalHints(kwm.dpy, c->win, &hints, &size)) {
		WARN("XGetWMNormalHints() failed");
		return;
	}

	/* base size */
	if (hints.flags & PBaseSize) {
		c->basew = hints.base_width;
		c->baseh = hints.base_height;
	} else if (hints.flags & PMinSize) {
		c->basew = hints.min_width;
		c->baseh = hints.min_height;
	} else {
		c->basew = c->baseh = 0;
	}

	/* resize steps */
	if (hints.flags & PResizeInc) {
		c->incw = hints.width_inc;
		c->inch = hints.height_inc;
	} else {
		c->incw = c->inch = 0;
	}

	/* minimum size */
	if (hints.flags & PMinSize) {
		c->minw = hints.min_width;
		c->minh = hints.min_height;
	} else if (hints.flags & PBaseSize) {
		c->minw = hints.base_width;
		c->minh = hints.base_height;
	} else {
		c->minw = c->minh = 0;
	}

	/* maximum size */
	if (hints.flags & PMaxSize) {
		c->maxw = hints.max_width;
		c->maxw = hints.max_height;
	} else {
		c->maxw = c->maxh = 0;
	}
}

