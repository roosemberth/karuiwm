#include "desktop.h"
#include "util.h"
#include "layout.h"

void
desktop_arrange(struct desktop *d)
{
	unsigned i, is = 0;
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
		       d->w, d->h);
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
	XRestackWindows(kwm.dpy, stack, (signed) d->nc);
	desktop_set_clientmask(d, CLIENTMASK);
}

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

void
desktop_delete(struct desktop *d)
{
	struct client *c;

	if (d->nc > 0) {
		WARN("deleting desktop containing clients");
		while (d->nc > 0) {
			c = d->clients[0];
			desktop_detach_client(d, c);
			client_delete(c);
		}
	}
	free(d);
}

void
desktop_detach_client(struct desktop *d, struct client *c)
{
	signed pos;
	unsigned oldpos, nextpos;

	pos = list_remove((void ***) &d->clients, &d->nc, c, "client list");
	if (pos < 0) {
		WARN("attempt to detach unhandled client");
		return;
	}
	oldpos = (unsigned) pos;
	if (oldpos < d->imaster)
		--d->imaster;
	if (c == d->selcli) {
		if (d->nc == 0) {
			d->selcli = NULL;
		} else {
			nextpos = MIN(oldpos, (unsigned) (d->nc - 1));
			d->selcli = d->clients[nextpos];
		}
	}
}

void
desktop_float_client(struct desktop *d, struct client *c, bool floating)
{
	unsigned pos;

	if (c->floating == floating)
		return;

	if (!desktop_locate_client(d, c, &pos)) {
		WARN("attempt to float unhandled window");
		return;
	}
	client_set_floating(c, floating);
	list_shift((void **) d->clients,
	           floating ? 0 : (unsigned) d->nc - 1, pos);
	d->imaster = (unsigned) ((signed) d->imaster + (floating ? 1 : -1));
	desktop_arrange(d);
}

void
desktop_fullscreen_client(struct desktop *d, struct client *c, bool fullscreen)
{
	if ((c->state == STATE_FULLSCREEN) == fullscreen)
		return;

	client_set_fullscreen(c, fullscreen);
	desktop_arrange(d);
}

bool
desktop_locate_client(struct desktop *d, struct client *c, unsigned *pos)
{
	unsigned i;

	for (i = 0; i < d->nc && d->clients[i] != c; ++i);
	if (i == d->nc)
		return false;
	if (pos != NULL)
		*pos = i;
	return true;
}

/* TODO replace by desktop_swap_clients */
bool
desktop_locate_neighbour(struct desktop *d, struct client *c, unsigned cpos,
                         signed dir, struct client **n, unsigned *npos)
{
	unsigned relsrc, reldst, dst;
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
	reldst = (unsigned) (relsrc + (size_t) ((signed) mod + dir))
	         % (unsigned) mod;
	dst = reldst + (unsigned) offset;
	if (npos != NULL)
		*npos = dst;
	if (n != NULL)
		*n = d->clients[dst];
	return true;
}

bool
desktop_locate_window(struct desktop *d, Window win,
                      struct client **c, unsigned *pos)
{
	unsigned i;
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

void
desktop_restack(struct desktop *d)
{
	unsigned i, i_fl, i_tl, nfloating;
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

void
desktop_set_clientmask(struct desktop *d, long mask)
{
	unsigned i;

	for (i = 0; i < d->nc; ++i)
		/* TODO move XSelectInput to client */
		XSelectInput(kwm.dpy, d->clients[i]->win, mask);
}

void
desktop_set_mfact(struct desktop *d, float mfact)
{
	d->mfact = mfact;
}

void
desktop_set_nmaster(struct desktop *d, size_t nmaster)
{
	d->nmaster = nmaster;
}

void
desktop_update_focus(struct desktop *d)
{
	unsigned i;

	if (d->nc == 0) {
		XSetInputFocus(kwm.dpy, kwm.root, RevertToPointerRoot,
		               CurrentTime);
		return;
	}
	for (i = 0; i < d->nc; ++i)
		client_set_focus(d->clients[i], d->clients[i] == d->selcli);
}

void
desktop_update_geometry(struct desktop *d,
                        signed x, signed y, unsigned w, unsigned h)
{
	d->x = x;
	d->y = y;
	d->w = w;
	d->h = h;
}
