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
	size_t ntiled = d->nc - d->imaster;
	struct client *c, **tiled = d->clients + d->imaster;
	Window stack[d->nc];

	if (d->nc == 0)
		return;

	desktop_set_clientmask(d, 0);
	for (i = 0; i < d->imaster; ++i) {
		c = d->clients[i];
		client_moveresize(c, c->x, c->y, c->w, c->h);
	}
	if (ntiled > 0) {
		/* TODO modular layout management */
		rstack(tiled, ntiled, MIN(d->nmaster, ntiled), d->mfact,
		       d->x, d->y, d->w, d->h);
	}
	for (i = 0; i < d->nc; ++i) {
		c = d->clients[i];
		if (c->state == STATE_FULLSCREEN) {
			client_moveresize(c, 0, 0, d->w, d->h);
			stack[is++] = c->win;
		}
	}
	for (i = 0; i < d->nc; ++i) {
		c = d->clients[i];
		if (c->state != STATE_FULLSCREEN)
			stack[is++] = c->win;
	}
	XRestackWindows(kwm.dpy, stack, (int signed) d->nc);
	desktop_set_clientmask(d, CLIENTMASK);
}

/* Attach a client to a desktop.
 */
void
desktop_attach_client(struct desktop *d, struct client *c)
{
	if (c->floating) {
		list_prepend((void ***) &d->clients, &d->nc, c, "client list");
		++d->imaster;
	} else {
		list_append((void ***) &d->clients, &d->nc, c, "client list");
	}
	d->selcli = c;
}

/* Properly delete a desktop and all its contained elements.
 */
void
desktop_delete(struct desktop *d)
{
	struct client *c;

	if (d->nc > 0) {
		WARN("deleting desktop that still contains clients");
		while (d->nc > 0) {
			c = d->clients[0];
			desktop_detach_client(d, c);
			client_delete(c);
		}
	}
	free(d);
}

/* Detach a client from a desktop.
 */
void
desktop_detach_client(struct desktop *d, struct client *c)
{
	int pos;
	int unsigned oldpos, nextpos;

	pos = list_remove((void ***) &d->clients, &d->nc, c, "client list");
	if (pos < 0) {
		WARN("attempt to detach unhandled client");
		return;
	}
	oldpos = (int unsigned) pos;
	if (oldpos < d->imaster)
		--d->imaster;
	if (c == d->selcli) {
		if (d->nc == 0) {
			d->selcli = NULL;
		} else {
			nextpos = MIN(oldpos, (int unsigned) (d->nc - 1));
			d->selcli = d->clients[nextpos];
		}
	}
}

/* Set the floating mode of a client in the context of a desktop.
 */
void
desktop_float_client(struct desktop *d, struct client *c, bool floating)
{
	int unsigned pos;

	if (c->floating == floating)
		return;

	if (!desktop_locate_client(d, c, &pos)) {
		WARN("attempt to float unhandled window");
		return;
	}
	client_set_floating(c, floating);
	list_shift((void **) d->clients,
	           floating ? 0 : (int unsigned) d->nc - 1, pos);
	d->imaster = (int unsigned) ((int signed) d->imaster
	                             + (floating ? 1 : -1));
	desktop_arrange(d);
}

/* Set the fullscreen mode of a client in the context of a desktop.
 */
void
desktop_fullscreen_client(struct desktop *d, struct client *c, bool fullscreen)
{
	if ((c->state == STATE_FULLSCREEN) == fullscreen)
		return;

	client_set_fullscreen(c, fullscreen);
	desktop_arrange(d);
}

/* Determine if a client is on a given desktop, and locate the client's position
 * in the desktop's client list.
 */
bool
desktop_locate_client(struct desktop *d, struct client *c, int unsigned *pos)
{
	int unsigned i;

	for (i = 0; i < d->nc && d->clients[i] != c; ++i);
	if (i == d->nc)
		return false;
	if (pos != NULL)
		*pos = i;
	return true;
}

/* TODO replace by desktop_swap_clients */
bool
desktop_locate_neighbour(struct desktop *d, struct client *c, int unsigned cpos,
                         int dir, struct client **n, int unsigned *npos)
{
	int unsigned relsrc, reldst, dst;
	size_t offset, mod;

	/* do not rely on client information */
	bool floating = cpos < d->imaster;

	if (d->nc < 2)
		return false;
	if (c->state == STATE_FULLSCREEN)
		return false;
	mod = floating ? d->imaster : d->nc - d->imaster;
	offset = floating ? 0 : d->imaster;
	relsrc = floating ? cpos : cpos - d->imaster;
	reldst = (int unsigned) (relsrc + (size_t) ((int signed) mod + dir))
	         % (int unsigned) mod;
	dst = reldst + (int unsigned) offset;
	if (npos != NULL)
		*npos = dst;
	if (n != NULL)
		*n = d->clients[dst];
	return true;
}

/* Determine if a window is contained in a given desktop, and locate both the
 * corresponding client and the client's position in the desktop's client list.
 */
bool
desktop_locate_window(struct desktop *d, Window win,
                      struct client **c, int unsigned *pos)
{
	int unsigned i;
	bool found = false;

	for (i = 0; i < d->nc; ++i) {
		if (d->clients[i]->win == win) {
			if (pos != NULL)
				*pos = i;
			if (c != NULL)
				*c = d->clients[i];
			found = true;
			break;
		}
	}
	return found;
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
	d->imaster = 0;
	d->nmaster = 1;
	d->nc = 0;
	d->clients = NULL;
	d->selcli = NULL;

	return d;
}

/* Check that the client stacking order is correct.
 */
void
desktop_restack(struct desktop *d)
{
	int unsigned i, i_fl, i_tl, nfloating;
	struct client *c, *restacked[d->nc];

	for (i = nfloating = 0; i < d->nc; ++i)
		nfloating += d->clients[i]->floating;
	for (i = i_fl = i_tl = 0; i < d->nc; ++i) {
		c = d->clients[i];
		restacked[c->floating ? i_fl++ : nfloating + i_tl++] = c;
	}
	for (i = 0; i < d->nc; ++i)
		d->clients[i] = restacked[i];
	d->imaster = nfloating;
}

/* Apply an event mask on all clients on a desktop.
 */
void
desktop_set_clientmask(struct desktop *d, long mask)
{
	int unsigned i;

	for (i = 0; i < d->nc; ++i)
		/* TODO move XSelectInput to client */
		XSelectInput(kwm.dpy, d->clients[i]->win, mask);
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

/* Set the X input focus to the currently selected client on a desktop.
 */
void
desktop_update_focus(struct desktop *d)
{
	int unsigned i;

	if (d->nc == 0) {
		XSetInputFocus(kwm.dpy, kwm.root, RevertToPointerRoot,
		               CurrentTime);
		return;
	}
	for (i = 0; i < d->nc; ++i)
		client_set_focus(d->clients[i], d->clients[i] == d->selcli);
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
