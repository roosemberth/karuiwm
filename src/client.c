#include "karuiwm.h"
#include "client.h"
#include <stdlib.h>
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define BORDERWIDTH 1

static bool checksizehints(struct client *, int unsigned *, int unsigned *);
static int get_name(struct client *);

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
		c->w = (int unsigned) wa.width;
		c->h = (int unsigned) wa.height;
	} else {
		c->x = c->y = 0;
		c->w = c->h = 1;
	}
	c->dirty = false;
	XSetWindowBorderWidth(kwm.dpy, c->win, BORDERWIDTH);
	client_refreshsizehints(c);
	c->name[0] = 0;
	client_refreshname(c);
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
client_moveresize(struct client *c, int x, int y, int unsigned w,
                  int unsigned h)
{
	client_move(c, x, y);
	client_resize(c, w, h);
}

void
client_refreshname(struct client *c)
{
	if (get_name(c) < 0 || c->name[0] == '\0')
		strcpy(c->name, "[broken]");
	c->name[BUFSIZ-1] = '\0';

	DEBUG("client(%lu)->name = %s", c->win, c->name);
}

void
client_refreshsizehints(struct client *c)
{
	long size;
	XSizeHints hints;

	if (!XGetWMNormalHints(kwm.dpy, c->win, &hints, &size)) {
		WARN("XGetWMNormalHints() failed");
		return;
	}

	/* base size */
	if (hints.flags & PBaseSize) {
		c->basew = (int unsigned) hints.base_width;
		c->baseh = (int unsigned) hints.base_height;
	} else if (hints.flags & PMinSize) {
		c->basew = (int unsigned) hints.min_width;
		c->baseh = (int unsigned) hints.min_height;
	} else {
		c->basew = c->baseh = 0;
	}

	/* resize steps */
	if (hints.flags & PResizeInc) {
		c->incw = (int unsigned) hints.width_inc;
		c->inch = (int unsigned) hints.height_inc;
	} else {
		c->incw = c->inch = 0;
	}

	/* minimum size */
	if (hints.flags & PMinSize) {
		c->minw = (int unsigned) hints.min_width;
		c->minh = (int unsigned) hints.min_height;
	} else if (hints.flags & PBaseSize) {
		c->minw = (int unsigned) hints.base_width;
		c->minh = (int unsigned) hints.base_height;
	} else {
		c->minw = c->minh = 0;
	}

	/* maximum size */
	if (hints.flags & PMaxSize) {
		c->maxw = (int unsigned) hints.max_width;
		c->maxw = (int unsigned) hints.max_height;
	} else {
		c->maxw = c->maxh = 0;
	}
}

void
client_resize(struct client *c, int unsigned w, int unsigned h)
{
	bool change = checksizehints(c, &w, &h);

	if (change) {
		c->w = w;
		c->h = h;
		XResizeWindow(kwm.dpy, c->win, c->w, c->h);
	}
}

void
client_setfloating(struct client *c, bool floating)
{
	c->floating = floating;
}

void
client_term(struct client *c)
{
	free(c);
}

static bool
checksizehints(struct client *c, int unsigned *w, int unsigned *h)
{
	int unsigned u; /* unit size */
	bool change = false;

	/* don't respect size hints for untiled or fullscreen windows */
	if (!c->floating || c->fullscreen) {
		return *w != c->w || *h != c->h;
	}

	if (*w != c->w) {
		if (c->basew > 0 && c->incw > 0) {
			u = (*w - c->basew)/c->incw;
			*w = c->basew + u*c->incw;
		}
		if (c->minw > 0)
			*w = MAX(*w, c->minw);
		if (c->maxw > 0)
			*w = MIN(*w, c->maxw);
		change |= *w != c->w;
	}
	if (*h != c->h) {
		if (c->baseh > 0 && c->inch > 0) {
			u = (*h - c->baseh)/c->inch;
			*h = c->baseh + u*c->inch;
		}
		if (c->minh > 0)
			*h = MAX(*h, c->minh);
		if (c->maxh > 0)
			*h = MIN(*h, c->maxh);
		change |= *h != c->h;
	}
	return change;
}

static int
get_name(struct client *c)
{
	XTextProperty xtp;
	int n;
	char **list;

	XGetTextProperty(kwm.dpy, c->win, &xtp, netatom[NetWMName]);
	if (!xtp.nitems)
		return -1;
	if (xtp.encoding == XA_STRING) {
		strncpy(c->name, (char const *) xtp.value, 255);
		//XFree(xtp.value);
		return 0;
	}
	if (XmbTextPropertyToTextList(kwm.dpy, &xtp, &list, &n) != Success
	|| n <= 0)
		return -1;
	strncpy(c->name, list[0], 255);
	//XFreeStringList(list);
	return 0;
}
