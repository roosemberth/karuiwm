#include "desktop.h"
#include "util.h"
#include "layout.h"

/* Propagate the client arrangement to the X server by applying the selected
 * layout and enforcing the window stack.
 */
void
desktop_arrange(struct desktop *d)
{
	int unsigned i, is = 0;
	struct client *c;
	Window stack[d->nt + d->nf];

	if (d->nt + d->nf == 0)
		return;

	desktop_set_clientmask(d, 0);
	/* fullscreen windows on top */
	for (i = 0; i < d->nf; ++i)
		if (d->floating[i]->state == STATE_FULLSCREEN)
			stack[is++] = d->floating[i]->win;
	for (i = 0; i < d->nt; ++i)
		if (d->tiled[i]->state == STATE_FULLSCREEN)
			stack[is++] = d->tiled[i]->win;

	/* non-fullscreen windows below */
	for (i = 0; i < d->nf; ++i) {
		c = d->floating[i];
		client_moveresize(c, c->x, c->y, c->w, c->h);
		if (c->state != STATE_FULLSCREEN)
			stack[is++] = c->win;
	}
	if (d->nt > 0) {
		/* TODO modular layout management */
		rstack(d->tiled, d->nt, MIN(d->nmaster, d->nt), d->mfact,
		       d->x, d->y, d->w, d->h);
		for (i = 0; i < d->nt; ++i) {
			if (d->tiled[i]->state != STATE_FULLSCREEN)
				stack[is++] = d->tiled[i]->win;
		}
	}
	XRestackWindows(kwm.dpy, stack, (int signed) (d->nt + d->nf));
	desktop_set_clientmask(d, CLIENTMASK);
}

/* Attach a client to a desktop.
 */
void
desktop_attach_client(struct desktop *d, struct client *c)
{
	if (c->floating)
		list_prepend((void ***) &d->floating, &d->nf, c,
		             "floating client list");
	else
		list_append((void ***) &d->tiled, &d->nt, c,
		            "tiled client list");
	d->selcli = c;
}

/* Determine if a client is on a given desktop, and locate the client's position
 * in the desktop's client list.
 */
bool
desktop_contains_client(struct desktop *d, struct client *c)
{
	int unsigned i;

	for (i = 0; i < d->nt; ++i)
		if (d->tiled[i] == c)
			return true;
	for (i = 0; i < d->nf; ++i)
		if (d->floating[i] == c)
			return true;
	return false;
}

/* Properly delete a desktop and all its contained elements.
 */
void
desktop_delete(struct desktop *d)
{
	struct client *c;

	if (d->nf > 0 || d->nt > 0) {
		WARN("deleting desktop that still contains clients");
		while (d->nt > 0) {
			c = d->tiled[0];
			desktop_detach_client(d, c);
			client_delete(c);
		}
		while (d->nf > 0) {
			c = d->floating[0];
			desktop_detach_client(d, c);
			client_delete(c);
		}
	}
	free(d);
}

/* Detach a client from a desktop.
 */
int
desktop_detach_client(struct desktop *d, struct client *c)
{
	int i;
	int unsigned pos;

	/* remove */
	if (c->floating)
		i = list_remove((void ***) &d->floating, &d->nf, c,
		                "floating client list");
	else
		i = list_remove((void ***) &d->tiled, &d->nt, c,
		                "tiled client list");
	if (i < 0) {
		WARN("client list inconsistency");
		return -1;
	}

	/* update selected client, if necessary */
	pos = (int unsigned) i;
	if (c == d->selcli) {
		if (c->floating) {
			d->selcli = d->nf > 0
			            ? d->floating[0]
			            : d->nt > 0 ? d->tiled[0] : NULL;
		} else {
			d->selcli = d->nt > 0
			            ? pos >= d->nt ? d->tiled[d->nt - 1]
			                         : d->tiled[i]
			            : d->nf > 0 ? d->floating[0] : NULL;
		}
	}
	return 0;
}

/* Set the floating mode of a client in the context of a desktop.
 */
void
desktop_float_client(struct desktop *d, struct client *c, bool floating)
{
	if (c->state == STATE_FULLSCREEN)
		return;
	if (desktop_detach_client(d, c) < 0)
		return;
	client_set_floating(c, floating);
	desktop_attach_client(d, c);
	desktop_arrange(d);
}

/* Set the fullscreen mode of a client in the context of a desktop.
 */
void
desktop_fullscreen_client(struct desktop *d, struct client *c, bool fullscreen)
{
	client_set_fullscreen(c, fullscreen);
	desktop_arrange(d);
}

/* Determine if a window is contained in a given desktop, and locate both the
 * corresponding client and the client's position in the desktop's client list.
 */
bool
desktop_locate_window(struct desktop *d, Window win, struct client **c)
{
	int unsigned i;

	for (i = 0; i < d->nt; ++i) {
		if (d->tiled[i]->win == win) {
			*c = d->tiled[i];
			return true;
		}
	}
	for (i = 0; i < d->nf; ++i) {
		if (d->floating[i]->win == win) {
			*c = d->floating[i];
			return true;
		}
	}
	return false;
}

/* Create and initialise a new desktop.
 */
struct desktop *
desktop_new(void)
{
	struct desktop *d = NULL;

	/* initialise desktop with default values */
	d = smalloc(sizeof(struct desktop), "desktop");
	d->mfact = 0.5;
	d->nmaster = 1;
	d->nt = d->nf = 0;
	d->tiled = d->floating = NULL;
	d->selcli = NULL;

	return d;
}

/* Apply an event mask on all clients on a desktop.
 * TODO move XSelectInput to client
 */
void
desktop_set_clientmask(struct desktop *d, long mask)
{
	int unsigned i;

	for (i = 0; i < d->nt; ++i)
		XSelectInput(kwm.dpy, d->tiled[i]->win, mask);
	for (i = 0; i < d->nf; ++i)
		XSelectInput(kwm.dpy, d->floating[i]->win, mask);
}

/* Set the factor of space the master area takes on a desktop (between 0.0 and
 * 1.0).
 */
void
desktop_set_mfact(struct desktop *d, float mfact)
{
	d->mfact = MAX(0.1f, MIN(0.9f, mfact));
}

/* Set the number of clients in the master area of the desktop.
 */
void
desktop_set_nmaster(struct desktop *d, size_t nmaster)
{
	d->nmaster = nmaster;
}

/* Move client up/down on the stack/in the layout.
 */
int
desktop_shift_client(struct desktop *d, struct client *c, int dir)
{
	size_t *nc;
	struct client ***clients;

	if (c->state == STATE_FULLSCREEN)
		return -1;

	/* list */
	if (c->floating) {
		clients = &d->floating;
		nc = &d->nf;
	} else {
		clients = &d->tiled;
		nc = &d->nt;
	}

	/* shift */
	return list_shift((void **) *clients, *nc, c, dir);
}

/* Move focus to next/previous client.
 */
void
desktop_step_focus(struct desktop *d, int dir)
{
	int pos;
	int unsigned oldpos, newpos, udir;
	struct client **oldlist, **newlist;
	size_t nc;

	if (d->selcli == NULL || d->selcli->state == STATE_FULLSCREEN)
		return;

	/* old position */
	if (d->selcli->floating) {
		oldlist = d->floating;
		nc = d->nf;
	} else {
		oldlist = d->tiled;
		nc = d->nt;
	}
	pos = list_index((void **) oldlist, nc, d->selcli);
	if (pos < 0)
		DIE("desktop has got an invalid selected client");
	oldpos = (int unsigned) pos;

	/* new position */
	while (dir < 0)
		dir += (int signed) (d->nt + d->nf);
	udir = (int unsigned) dir;
	while (udir >= d->nt + d->nf)
		udir -= (int unsigned) (d->nt + d->nf);
	newpos = oldpos + udir;
	if (newpos >= nc) {
		newlist = (oldlist == d->tiled) ? d->floating : d->tiled;
		if (newlist == NULL)
			newlist = oldlist;
		newpos -= (int unsigned) nc;
	} else {
		newlist = oldlist;
	}
	d->selcli = newlist[newpos];

	desktop_update_focus(d);
}

/* Set the X input focus to the currently selected client on a desktop.
 * TODO abstract root window as a client
 */
void
desktop_update_focus(struct desktop *d)
{
	int unsigned i;

	if (d->nt + d->nf == 0) {
		XSetInputFocus(kwm.dpy, kwm.root, RevertToPointerRoot,
		               CurrentTime);
		return;
	}
	for (i = 0; i < d->nt; ++i)
		client_set_focus(d->tiled[i], d->tiled[i] == d->selcli);
	for (i = 0; i < d->nf; ++i)
		client_set_focus(d->floating[i], d->floating[i] == d->selcli);
}

/* Update the dimension at which the desktop is to be displayed.
 */
void
desktop_update_geometry(struct desktop *d,
                        int x, int y, int unsigned w, int unsigned h)
{
	d->x = x;
	d->y = y;
	d->w = w;
	d->h = h;
}

/* "Zoom" selected client: if it is not master, swap it with master, otherwise
 * make and focus next below client.
 */
void
desktop_zoom(struct desktop *d)
{
	int pos;

	if (d->nt < 2 || d->selcli->floating
	|| d->selcli->state == STATE_FULLSCREEN)
		return;

	pos = list_index((void **) d->tiled, d->nt, d->selcli);
	if (pos == 0) {
		/* window is at the top: swap with next below */
		d->tiled[0] = d->tiled[1];
		d->tiled[1] = d->selcli;
		d->selcli = d->tiled[0];
		desktop_update_focus(d);
	} else {
		/* window is is somewhere else: swap with top */
		d->tiled[pos] = d->tiled[0];
		d->tiled[0] = d->selcli;
	}
	desktop_arrange(d);
}
