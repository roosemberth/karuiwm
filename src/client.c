#include "karuiwm.h"
#include "client.h"
#include "util.h"
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>

#define BORDERWIDTH 1

static bool check_sizehints(struct client *c, int unsigned *w, int unsigned *h);
static Atom get_atom(Window win, Atom property);
static int get_name(char *buf, Window win);
static int send_atom(Window win, Atom atom);

/* Properly delete a client all its contained elements.
 */
void
client_delete(struct client *c)
{
	sfree(c);
}

/* Make the client trigger button-click events for given buttons.
 */
void
client_grab_buttons(struct client *c, size_t nb, struct button *buttons)
{
	int unsigned i;

	XUngrabButton(kwm.dpy, AnyButton, AnyModifier, c->win);
	for (i = 0; i < nb; ++i)
		XGrabButton(kwm.dpy, buttons[i].button, buttons[i].mod,
		            c->win, False, BUTTONMASK, GrabModeAsync,
		            GrabModeAsync, None, None);
}

/* Close a client's window.
 */
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

/* Move a client to a given position.
 */
void
client_move(struct client *c, int x, int y)
{
	if (c->x == x && c->y == y)
		return;
	c->x = x;
	c->y = y;
	if (c->floating && c->state == STATE_NORMAL) {
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

/* Create and initialise a new client.
 */
struct client *
client_new(Window win)
{
	struct client *c;
	XWindowAttributes wa;

	/* ignore buggy windows and windows with override_redirect */
	if (!XGetWindowAttributes(kwm.dpy, win, &wa)) {
		WARN("XGetWindowAttributes() failed for window %lu", win);
		return NULL;
	}
	if (wa.override_redirect)
		return NULL;

	/* initialise client with default values */
	c = smalloc(sizeof(struct client), "client");
	c->win = win;
	c->floating = false;
	c->dialog = false;
	c->state = STATE_NORMAL;
	c->w = c->h = c->floatw = c->floath = 1;
	c->x = c->y = c->floatx = c->floaty = 0;

	/* query client properties */
	client_query_dialog(c);
	client_query_dimension(c);
	client_query_fullscreen(c);
	client_query_name(c);
	client_query_sizehints(c);
	client_query_transient(c);

	return c;
}

/* Check and update if a client is a dialog box.
 */
void
client_query_dialog(struct client *c)
{
	Atom type;

	type = get_atom(c->win, netatom[NetWMWindowType]);
	client_set_dialog(c, type == netatom[NetWMWindowTypeDialog]);
}

/* Check and update client-requested dimension information.
 */
void
client_query_dimension(struct client *c)
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

/* Check and update if a client is fullscreen.
 */
void
client_query_fullscreen(struct client *c)
{
	Atom state;

	state = get_atom(c->win, netatom[NetWMState]);
	client_set_fullscreen(c, state == netatom[NetWMStateFullscreen]);
}

/* Update a client's name.
 */
void
client_query_name(struct client *c)
{
	if (get_name(c->name, c->win) < 0 || c->name[0] == '\0')
		strcpy(c->name, "[broken]");
	c->name[BUFSIZ-1] = '\0';
}

/* Check and update client-requested size hints.
 */
void
client_query_sizehints(struct client *c)
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

/* Check and update if the client is a transient for another client.
 */
void
client_query_transient(struct client *c)
{
	Window trans = 0;

	if (!XGetTransientForHint(kwm.dpy, c->win, &trans))
		return;
	DEBUG("window %lu is transient", c->win);
}

/* Change the size of a client.
 */
void
client_resize(struct client *c, int unsigned w, int unsigned h)
{
	bool change = check_sizehints(c, &w, &h);

	if (change) {
		c->w = w;
		c->h = h;
		if (c->floating && c->state == STATE_NORMAL) {
			c->floatw = w;
			c->floath = h;
		}
		XResizeWindow(kwm.dpy, c->win, c->w, c->h);
	}
}

/* Set the thickness of a client's border.
 */
void
client_set_border(struct client *c, int unsigned border)
{
	c->border = border;
	XSetWindowBorderWidth(kwm.dpy, c->win, border);
}

/* Set the dialog mode of a client.
 */
void
client_set_dialog(struct client *c, bool dialog)
{
	c->dialog = dialog;
	client_set_floating(c, c->dialog || c->floating);
}

/* Set the floating mode of a client.
 */
void
client_set_floating(struct client *c, bool floating)
{
	c->floating = floating;
	if (c->floating)
		client_moveresize(c, c->floatx, c->floaty, c->floatw,c->floath);
}

/* Set/unset the X input focus on a client.
 */
void
client_set_focus(struct client *c, bool focus)
{
	XSetWindowBorder(kwm.dpy, c->win, focus ? CBORDERSEL : CBORDERNORM);
	if (focus)
		XSetInputFocus(kwm.dpy, c->win, RevertToPointerRoot,
		               CurrentTime);
}

/* Set the fullscreen mode of a client.
 */
void
client_set_fullscreen(struct client *c, bool fullscreen)
{
	c->state = fullscreen ? STATE_FULLSCREEN : STATE_NORMAL;
	client_set_border(c, fullscreen ? 0 : BORDERWIDTH);
}

/* Check and enforce client-requested size hints on a server-requested client
 * resize.
 */
static bool
check_sizehints(struct client *c, int unsigned *w, int unsigned *h)
{
	int unsigned u; /* unit size */
	bool change = false;

	/* don't respect size hints for untiled or fullscreen windows */
	if (!c->floating || c->state == STATE_FULLSCREEN)
		return *w != c->w || *h != c->h;

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

/* Get an Atom from a client.
 */
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

/* Get a client's window's name.
 */
static int
get_name(char *buf, Window win)
{
	XTextProperty xtp;
	int n, ret, fret = 0;
	char **list;

	XGetTextProperty(kwm.dpy, win, &xtp, netatom[NetWMName]);
	if (!xtp.nitems)
		return -1;
	if (xtp.encoding == XA_STRING) {
		strncpy(buf, (char const *) xtp.value, 255);
		goto get_name_out;
	}
	ret = XmbTextPropertyToTextList(kwm.dpy, &xtp, &list, &n);
	if (ret != Success || n <= 0) {
		fret = -1;
		goto get_name_out;
	}
	strncpy(buf, list[0], 255);
	XFreeStringList(list);
 get_name_out:
	XFree(xtp.value);
	return fret;
}

static int
send_atom(Window win, Atom atom)
{
	int n;
	Atom *supported;
	bool exists = false;
	XEvent ev;

	if (XGetWMProtocols(kwm.dpy, win, &supported, &n)) {
		while (!exists && n-- > 0)
			exists = supported[n] == atom;
		XFree(supported);
	}
	if (!exists)
		return -1;

	ev.type = ClientMessage;
	ev.xclient.window = win;
	ev.xclient.message_type = wmatom[WMProtocols];
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = (long) atom;
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(kwm.dpy, win, false, NoEventMask, &ev);
	return 0;
}
