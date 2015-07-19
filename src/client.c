#include "karuiwm.h"
#include "client.h"
#include "util.h"
#include <string.h>
#include <X11/Xutil.h>
#include <X11/Xatom.h>
#include <stdarg.h>

#define BORDERWIDTH 1

static int check_sizehints(struct client *c, int unsigned *w, int unsigned *h);
static int get_name(char *buf, Window win);
static void massacre(struct client *c);

static int
check_sizehints(struct client *c, int unsigned *w, int unsigned *h)
{
	int unsigned u; /* unit size */
	bool change = false;

	/* don't respect size hints for untiled or fullscreen windows */
	if (!c->floating || c->state == STATE_FULLSCREEN)
		return *w == c->w && *h == c->h ? 0 : -1;

	if (*w != c->w) {
		if (c->basew > 0 && c->incw > 0) {
			u = (*w - c->basew)/c->incw;
			*w = c->basew + u*c->incw;
		}
		*w = MAX(*w, MAX(c->minw, 1));
		if (c->maxw > 0)
			*w = MIN(*w, c->maxw);
		change |= *w != c->w;
	}
	if (*h != c->h) {
		if (c->baseh > 0 && c->inch > 0) {
			u = (*h - c->baseh)/c->inch;
			*h = c->baseh + u*c->inch;
		}
		*h = MAX(*h, MAX(c->minh, 1));
		if (c->maxh > 0)
			*h = MIN(*h, c->maxh);
		change |= *h != c->h;
	}
	return change ? -1 : 0;
}

void
client_delete(struct client *c)
{
	if (c->supported != NULL)
		sfree(c->supported);
	sfree(c);
}

void
client_grab_buttons(struct client *c, size_t nb, struct button *buttons)
{
	int unsigned i;

	XUngrabButton(karuiwm.dpy, AnyButton, AnyModifier, c->win);
	for (i = 0; i < nb; ++i)
		XGrabButton(karuiwm.dpy, buttons[i].button, buttons[i].mod,
		            c->win, False, BUTTONMASK, GrabModeAsync,
		            GrabModeAsync, None, None);
}

void
client_kill(struct client *c)
{
	int i;

	if (!client_supports_atom(c, atoms[WM_DELETE_WINDOW])) {
		WARN("WM_DELETE_WINDOW not supported by %lu", c->win);
		massacre(c);
		return;
	}
	i = client_send_atom(c, 2, atoms[WM_PROTOCOLS],
	                           atoms[WM_DELETE_WINDOW]);
	if (i < 0) {
		WARN("could not send WM_DELETE_WINDOW to %lu", c->win);
		massacre(c);
	}
}

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
	XMoveWindow(karuiwm.dpy, c->win, c->x, c->y);
}

void
client_moveresize(struct client *c, int x, int y,
                  int unsigned w, int unsigned h)
{
	client_move(c, x, y);
	client_resize(c, w, h);
}

struct client *
client_new(Window win)
{
	struct client *c;
	XWindowAttributes wa;

	/* ignore buggy windows and windows with override_redirect */
	if (!XGetWindowAttributes(karuiwm.dpy, win, &wa)) {
		WARN("XGetWindowAttributes() failed for window %lu", win);
		return NULL;
	}
	if (wa.override_redirect)
		return NULL;

	/* initialise client with default values */
	c = smalloc(sizeof(struct client), "client");
	c->next = c->prev = NULL;
	c->win = win;
	c->floating = false;
	c->dialog = false;
	c->state = STATE_NORMAL;
	c->w = c->h = c->floatw = c->floath = 0;
	c->x = c->y = c->floatx = c->floaty = 0;
	c->nsup = 0;
	c->supported = NULL;

	/* query client properties */
	client_query_dialog(c);
	client_query_sizehints(c);
	client_query_dimension(c);
	client_query_fullscreen(c);
	client_query_name(c);
	client_query_supported_atoms(c);
	client_query_transient(c);

	return c;
}

Atom
client_query_atom(struct client *c, Atom property)
{
	int ret, i;
	int long unsigned lu;
	char unsigned *atomp = NULL;
	Atom a, atom = None;

	ret = XGetWindowProperty(karuiwm.dpy, c->win, property, 0L,
	                         sizeof(Atom), False, XA_ATOM, &a, &i, &lu, &lu,
	                         &atomp);
	if (ret != Success) {
		WARN("%lu: could not get property", c->win);
	} else if (atomp != NULL) {
		atom = *(Atom *) atomp;
		XFree(atomp);
	}
	return atom;
}

void
client_query_dialog(struct client *c)
{
	Atom type;

	type = client_query_atom(c, netatoms[_NET_WM_WINDOW_TYPE]);
	client_set_dialog(c, type == netatoms[_NET_WM_WINDOW_TYPE_DIALOG]);
}

void
client_query_dimension(struct client *c)
{
	Window root;
	int unsigned u;

	if (!XGetGeometry(karuiwm.dpy, c->win, &root, &c->floatx, &c->floaty,
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
client_query_fullscreen(struct client *c)
{
	Atom state;

	state = client_query_atom(c, netatoms[_NET_WM_STATE]);
	client_set_fullscreen(c, state == netatoms[_NET_WM_STATE_FULLSCREEN]);
}

void
client_query_name(struct client *c)
{
	if (get_name(c->name, c->win) < 0 || c->name[0] == '\0')
		strcpy(c->name, "[broken]");
	c->name[BUFSIZ-1] = '\0';
}

void
client_query_sizehints(struct client *c)
{
	long size;
	XSizeHints hints;

	if (!XGetWMNormalHints(karuiwm.dpy, c->win, &hints, &size)) {
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
client_query_supported_atoms(struct client *c)
{
	int unsigned i, nsup;
	Atom *sup;

	if (c->supported != NULL) {
		free(c->supported);
		nsup = 0;
	}
	if (!XGetWMProtocols(karuiwm.dpy, c->win, &sup, (int signed *) &nsup)) {
		WARN("XGetWMProtocols() failed on %lu", c->win);
		return;
	}
	c->nsup = (size_t) nsup;
	c->supported = scalloc(nsup, sizeof(Atom), "supported atoms list");
	for (i = 0; i < nsup; ++i)
		c->supported[i] = sup[i];
	XFree(sup);
}

void
client_query_transient(struct client *c)
{
	Window trans = 0;

	if (!XGetTransientForHint(karuiwm.dpy, c->win, &trans))
		return;
	DEBUG("window %lu is transient", c->win);
}

void
client_resize(struct client *c, int unsigned w, int unsigned h)
{
	if (check_sizehints(c, &w, &h) == 0)
		return;
	c->w = w;
	c->h = h;
	if (c->floating && c->state == STATE_NORMAL) {
		c->floatw = w;
		c->floath = h;
	}
	XResizeWindow(karuiwm.dpy, c->win, c->w, c->h);
}

int
client_send_atom(struct client *c, size_t natoms, ...)
{
	int retval = 0;
	int unsigned i;
	va_list args;
	XEvent ev;

	if (natoms == 0 || natoms > 4) {
		WARN("attempt to send %zu atoms", natoms);
		return -1;
	}
	va_start(args, natoms);

	ev.type = ClientMessage;
	ev.xclient.window = c->win;
	ev.xclient.format = 32;
	ev.xclient.message_type = va_arg(args, Atom);
	for (i = 0; i < natoms - 1; ++i)
		ev.xclient.data.l[i] = (int long) va_arg(args, Atom);
	ev.xclient.data.l[i] = CurrentTime;

	if (!XSendEvent(karuiwm.dpy, c->win, false, NoEventMask, &ev))
		WARN("could not send event to %lu", c->win);

	va_end(args);
	return retval;
}

void
client_set_border(struct client *c, int unsigned border)
{
	c->border = border;
	XSetWindowBorderWidth(karuiwm.dpy, c->win, border);
}

void
client_set_dialog(struct client *c, bool dialog)
{
	c->dialog = dialog;
	if (dialog)
		client_set_floating(c, dialog);
}

void
client_set_floating(struct client *c, bool floating)
{
	c->floating = floating;
}

void
client_set_focus(struct client *c, bool focus)
{
	XSetWindowBorder(karuiwm.dpy, c->win, focus ? CBORDERSEL : CBORDERNORM);
	if (focus)
		XSetInputFocus(karuiwm.dpy, c->win, RevertToPointerRoot,
		               CurrentTime);
}

void
client_set_fullscreen(struct client *c, bool fullscreen)
{
	c->state = fullscreen ? STATE_FULLSCREEN : STATE_NORMAL;
	client_set_border(c, fullscreen ? 0 : BORDERWIDTH);
}

void
client_set_visibility(struct client *c, bool visible)
{
	Atom action;

	c->visible = visible;
	if (c->visible)
		XMapWindow(karuiwm.dpy, c->win);
	else
		XUnmapWindow(karuiwm.dpy, c->win);

	action = visible ? _NET_WM_STATE_ADD : _NET_WM_STATE_REMOVE;
	client_send_atom(c, 3, netatoms[_NET_WM_STATE], action,
	                 netatoms[_NET_WM_STATE_HIDDEN]);
}

bool
client_supports_atom(struct client *c, Atom atom)
{
	int unsigned i;

	for (i = 0; i < c->nsup; ++i)
		if (c->supported[i] == atom)
			return true;
	return false;
}

static int
get_name(char *buf, Window win)
{
	XTextProperty xtp;
	int n, ret, fret = 0;
	char **list;

	XGetTextProperty(karuiwm.dpy, win, &xtp, netatoms[_NET_WM_NAME]);
	if (!xtp.nitems)
		return -1;
	if (xtp.encoding == XA_STRING) {
		strncpy(buf, (char const *) xtp.value, 255);
		goto get_name_out;
	}
	ret = XmbTextPropertyToTextList(karuiwm.dpy, &xtp, &list, &n);
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

static void
massacre(struct client *c)
{
	XGrabServer(karuiwm.dpy);
	XSetCloseDownMode(karuiwm.dpy, DestroyAll);
	XKillClient(karuiwm.dpy, c->win);
	XSync(karuiwm.dpy, false);
	XUngrabServer(karuiwm.dpy);
}
