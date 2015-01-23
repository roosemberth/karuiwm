#include "karuiwm.h"
#include "client.h"
#include "util.h"
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define BORDERWIDTH 1

static bool checksizehints(struct client *c, int unsigned *w, int unsigned *h);
static Atom get_atom(Window win, Atom property);
static int get_name(char *buf, Window win);
static void grab_buttons(struct client *c, bool focus);
static int send_atom(Window win, Atom atom);

/* TODO this is a hack, the same buttons are defined in karuiwm.c:
 * Make static grab_buttons to public client_grab_button(s) and let KWM pass the
 * button(s) as argument.
 * Button definitions clearly don't belong here.
 */
static struct button const buttons[] = {
	{ MODKEY,           Button1,    movemouse,   { 0 } },
};

void
client_kill(struct client *c)
{
	if (send_atom(c->win, wmatom[WMDeleteWindow]) < 0) {
		/* massacre! */
		XGrabServer(kwm.dpy);
		XSetCloseDownMode(kwm.dpy, DestroyAll);
		XKillClient(kwm.dpy, c->win);
		XSync(kwm.dpy, false);
		XUngrabServer(kwm.dpy);
	}
}

struct client *
client_init(Window win)
{
	struct client *c;
	XWindowAttributes wa;

	/* ignore buggy windows and windows with override_redirect */
	if (!XGetWindowAttributes(kwm.dpy, win, &wa)) {
		WARN("XGetWindowAttributes() failed for window %d", win);
		return NULL;
	}
	if (wa.override_redirect)
		return NULL;

	/* initialise client with default values */
	c = smalloc(sizeof(struct client), "client");
	c->win = win;
	c->floating = false;

	/* query client properties */
	client_querydialog(c);
	client_querydimension(c);
	client_queryfullscreen(c);
	client_queryname(c);
	client_querysizehints(c);
	client_querytransient(c);

	return c;
}

void
client_move(struct client *c, int x, int y)
{
	c->x = x;
	c->y = y;
	if (c->floating) {
		c->floatx = x;
		c->floaty = y;
	}
	XMoveWindow(kwm.dpy, c->win, c->x, c->y);
}

void
client_moveresize(struct client *c, int x, int y,
                  int unsigned w, int unsigned h)
{
	client_move(c, x, y);
	client_resize(c, w, h);
}

void
client_querydialog(struct client *c)
{
	Atom type;

	type = get_atom(c->win, netatom[NetWMWindowType]);
	client_setdialog(c, type == netatom[NetWMWindowTypeDialog]);
}

void
client_querydimension(struct client *c)
{
	Window root;
	int unsigned u;

	if (!XGetGeometry(kwm.dpy, c->win, &root, &c->floatx, &c->floaty,
	                  &c->floatw, &c->floath, &c->border, &u)) {
		WARN("window %lu: could not get geometry", c->win);
		return;
	}
	if (c->floating) {
		c->x = c->floatx;
		c->y = c->floaty;
		c->w = c->floatw;
		c->h = c->floath;
	}
}

void
client_queryfullscreen(struct client *c)
{
	Atom state;

	state = get_atom(c->win, netatom[NetWMState]);
	client_setfullscreen(c, state == netatom[NetWMStateFullscreen]);
}

void
client_queryname(struct client *c)
{
	if (get_name(c->name, c->win) < 0 || c->name[0] == '\0')
		strcpy(c->name, "[broken]");
	c->name[BUFSIZ-1] = '\0';
}

void
client_querysizehints(struct client *c)
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
client_querytransient(struct client *c)
{
	Window trans = 0;

	if (!XGetTransientForHint(kwm.dpy, c->win, &trans))
		return;
	DEBUG("window %lu is transient", c->win);
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
client_setborder(struct client *c, int unsigned border)
{
	c->border = border;
	XSetWindowBorderWidth(kwm.dpy, c->win, border);
}

void
client_setdialog(struct client *c, bool dialog)
{
	c->dialog = dialog;
	client_setfloating(c, c->dialog || c->floating);
}

void
client_setfloating(struct client *c, bool floating)
{
	c->floating = floating;
	if (c->floating)
		client_moveresize(c, c->floatx, c->floaty, c->floatw,c->floath);
}

void
client_setfocus(struct client *c, bool focus)
{
	XSetWindowBorder(kwm.dpy, c->win, focus ? CBORDERSEL : CBORDERNORM);
	if (focus)
		XSetInputFocus(kwm.dpy, c->win, RevertToPointerRoot,
		               CurrentTime);
	grab_buttons(c, true);
}

void
client_setfullscreen(struct client *c, bool fullscreen)
{
	c->fullscreen = fullscreen;
	client_setborder(c, fullscreen ? 0 : BORDERWIDTH);
}

void
client_term(struct client *c)
{
	sfree(c);
}

/* implementation (static) */
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

static Atom
get_atom(Window win, Atom property)
{
	int ret, i;
	int long unsigned lu;
	char unsigned *atomp = NULL;
	Atom a, atom = None;

	ret = XGetWindowProperty(kwm.dpy, win, property, 0L, sizeof(Atom),
	                         False, XA_ATOM, &a, &i, &lu, &lu, &atomp);
	if (ret != Success) {
		WARN("%lu: could not get property", win);
	} else if (atomp != NULL) {
		atom = *(Atom *) atomp;
		XFree(atomp);
	}
	return atom;
}

static int
get_name(char *buf, Window win)
{
	XTextProperty xtp;
	int n;
	char **list;

	XGetTextProperty(kwm.dpy, win, &xtp, netatom[NetWMName]);
	if (!xtp.nitems)
		return -1;
	if (xtp.encoding == XA_STRING) {
		strncpy(buf, (char const *) xtp.value, 255);
		//XFree(xtp.value);
		return 0;
	}
	if (XmbTextPropertyToTextList(kwm.dpy, &xtp, &list, &n) != Success
	|| n <= 0)
		return -1;
	strncpy(buf, list[0], 255);
	//XFreeStringList(list);
	return 0;
}

static int
send_atom(Window win, Atom atom)
{
	int n;
	Atom *supported;
	bool exists = false;
	XEvent ev;

	if (XGetWMProtocols(kwm.dpy, win, &supported, &n)) {
		while (!exists && n--)
			exists = supported[n] == atom;
		XFree(supported);
	}
	if (!exists)
		return -1;

	ev.type = ClientMessage;
	ev.xclient.window = win;
	ev.xclient.message_type = wmatom[WMProtocols];
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = (int long) atom;
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(kwm.dpy, win, false, NoEventMask, &ev);
	return 0;
}

static void
grab_buttons(struct client *c, bool focus)
{
	int unsigned i;

	XUngrabButton(kwm.dpy, AnyButton, AnyModifier, c->win);
	if (focus) {
		/* only grab special buttons */
		for (i = 0; i < LENGTH(buttons); ++i) {
			XGrabButton(kwm.dpy, buttons[i].button, buttons[i].mod,
			            c->win, False, BUTTONMASK, GrabModeAsync,
			            GrabModeAsync, None, None);
		}
	} else {
		/* grab all buttons */
		XGrabButton(kwm.dpy, AnyButton, AnyModifier, c->win, False,
		            BUTTONMASK, GrabModeAsync, GrabModeAsync, None,
		            None);
	}
}
