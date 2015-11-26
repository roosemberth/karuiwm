#include "desktop.h"
#include "util.h"
#include "list.h"
#include "layout.h"

static struct client *get_head(struct desktop *d, struct client *c);
static struct client *get_last(struct desktop *d, struct client *c);
static struct client *get_neighbour(struct client *c, enum list_direction dir);

void
desktop_arrange(struct desktop *d)
{
	int unsigned i, is = 0;
	struct client *c;
	Window stack[d->nt + d->nf];

	if (d->tiled == NULL && d->floating == NULL)
		return;

	desktop_set_clientmask(d, 0);

	/* fullscreen windows on top */
	for (i = 0, c = d->floating; i < d->nf; ++i, c = c->next)
		if (c->state == STATE_FULLSCREEN)
			stack[is++] = c->win;
	for (i = 0, c = d->tiled; i < d->nt; ++i, c = c->next)
		if (c->state == STATE_FULLSCREEN)
			stack[is++] = c->win;

	/* non-fullscreen windows below */
	for (i = 0, c = d->floating; i < d->nf; ++i, c = c->next) {
		client_moveresize(c, c->floatx, c->floaty, c->floatw, c->floath);
		if (c->state != STATE_FULLSCREEN)
			stack[is++] = c->win;
	}
	if (d->nt > 0) {
		/* FIXME strut (left, right, bottom, top) != 0 break this */
		d->sellayout->apply(d->tiled, d->nt, MIN(d->nmaster, d->nt),
		                    d->mfact, d->monitor->x, d->monitor->y,
		                    d->monitor->w, d->monitor->h);
		stack[is++] = d->seltiled->win;
		for (i = 0, c = d->tiled; i < d->nt; ++i, c = c->next)
			if (c->state != STATE_FULLSCREEN && c != d->seltiled)
				stack[is++] = c->win;
	}
	XRestackWindows(karuiwm.dpy, stack, (int signed) (d->nt + d->nf));
	desktop_set_clientmask(d, CLIENTMASK);
}

void
desktop_attach_client(struct desktop *d, struct client *c)
{
	if (c->floating) {
		LIST_PREPEND(&d->floating, c);
		++d->nf;
	} else {
		LIST_APPEND(&d->tiled, c);
		++d->nt;
		d->seltiled = c;
	}
	d->selcli = c;
	c->desktop = d;
}

void
desktop_delete(struct desktop *d)
{
	struct client *c;

	if (d->nt + d->nf > 0) {
		if (!karuiwm.restarting)
			WARN("deleting desktop that still contains clients");
		while (d->nt > 0) {
			c = d->tiled;
			desktop_detach_client(d, c);
			client_delete(c);
		}
		while (d->nf > 0) {
			c = d->floating;
			desktop_detach_client(d, c);
			client_delete(c);
		}
	}
	free(d);
}

void
desktop_detach_client(struct desktop *d, struct client *c)
{
	struct client *next = d->selcli;

	if (next == c) {
		if (next->next != c) {
			next = c->next;
		} else {
			next = c->floating ? d->tiled : d->floating;
		}
	}
	if (c->floating) {
		LIST_REMOVE(&d->floating, c);
		--d->nf;
	} else {
		LIST_REMOVE(&d->tiled, c);
		--d->nt;
	}
	d->selcli = next;
	if (!c->floating)
		d->seltiled = d->selcli;
}

void
desktop_float_client(struct desktop *d, struct client *c, bool floating)
{
	if (c == NULL || c->state != STATE_NORMAL)
		return;
	desktop_detach_client(d, c);
	client_set_floating(c, floating);
	desktop_attach_client(d, c);
}

void
desktop_focus_client(struct desktop *d, struct client *c)
{
	d->selcli = c;
	if (!c->floating)
		d->seltiled = c;
	desktop_update_focus(d);
}

void
desktop_fullscreen_client(struct desktop *d, struct client *c, bool fullscreen)
{
	(void) d;

	client_set_fullscreen(c, fullscreen);
}

void
desktop_kill_client(struct desktop *d)
{
	if (d->selcli == NULL)
		return;
	client_kill(d->selcli);
}

int
desktop_locate_window(struct desktop *d, struct client **c, Window win)
{
	struct client *it;
	int unsigned i;

	if (d == NULL)
		return -1;
	for (i = 0, it = d->tiled; i < d->nt; ++i, it = it->next) {
		if (it->win == win) {
			*c = it;
			return 0;
		}
	}
	for (i = 0, it = d->floating; i < d->nf; ++i, it = it->next) {
		if (it->win == win) {
			*c = it;
			return 0;
		}
	}
	return -1;
}

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
	d->selcli = d->seltiled = NULL;
	d->sellayout = layouts;
	d->focus = false;
	d->workspace = NULL;
	d->monitor = NULL;
	return d;
}

void
desktop_set_clientmask(struct desktop *d, long mask)
{
	struct client *it;
	int unsigned i;

	/* TODO move XSelectInput to client */
	for (i = 0, it = d->tiled; i < d->nt; ++i, it = it->next)
		XSelectInput(karuiwm.dpy, it->win, mask);
	for (i = 0, it = d->floating; i < d->nf; ++i, it = it->next)
		XSelectInput(karuiwm.dpy, it->win, mask);
	XSync(karuiwm.dpy, karuiwm.screen);
}

void
desktop_set_focus(struct desktop *d, bool focus)
{
	d->focus = focus;
	desktop_update_focus(d);
}

void
desktop_set_mfact(struct desktop *d, float mfact)
{
	d->mfact = MAX(0.1f, MIN(0.9f, mfact));
}

void
desktop_set_nmaster(struct desktop *d, size_t nmaster)
{
	d->nmaster = nmaster;
}

void
desktop_shift_client(struct desktop *d, int dir)
{
	struct client *this, *other;

	this = d->selcli;
	if (this == NULL)
		return;
	other = get_neighbour(this, dir);
	LIST_SWAP(this->floating ? &d->floating : &d->tiled, this, other);
}

void
desktop_show(struct desktop *d, struct monitor *m)
{
	int unsigned i;
	struct client *c;
	bool visible = m != NULL;
	d->monitor = m;

	desktop_set_clientmask(d, 0);
	for (i = 0, c = d->tiled; i < d->nt; ++i, c = c->next)
		client_set_visibility(c, visible);
	for (i = 0, c = d->floating; i < d->nf; ++i, c = c->next)
		client_set_visibility(c, visible);
	desktop_set_clientmask(d, CLIENTMASK);
	if (visible)
		desktop_arrange(d);
}

void
desktop_step_client(struct desktop *d, enum list_direction dir)
{
	if (d->selcli == NULL || d->selcli->state == STATE_FULLSCREEN)
		return;

	desktop_focus_client(d, get_neighbour(d->selcli, dir));
}

void
desktop_step_layout(struct desktop *d, enum list_direction dir)
{
	d->sellayout = (dir == PREV) ? d->sellayout->prev : d->sellayout->next;
}

void
desktop_update_focus(struct desktop *d)
{
	int unsigned i;
	struct client *c;

	if (d->nt + d->nf == 0) {
		XSetInputFocus(karuiwm.dpy, karuiwm.root, RevertToPointerRoot,
		               CurrentTime);
		return;
	}
	for (i = 0, c = d->tiled; i < d->nt; ++i, c = c->next)
		client_set_focus(c, d->focus && c == d->selcli);
	for (i = 0, c = d->floating; i < d->nf; ++i, c = c->next)
		client_set_focus(c, d->focus && c == d->selcli);
}

void
desktop_zoom(struct desktop *d)
{
	if (d->tiled == NULL || d->tiled->next == NULL)
		return;
	if (d->selcli->floating || d->selcli->state != STATE_NORMAL)
		return;

	if (d->selcli == d->tiled) {
		/* window is at the top: swap with next below */
		LIST_SWAP(&d->tiled, d->tiled, d->tiled->next);
		d->selcli = d->seltiled = d->tiled;
		desktop_update_focus(d);
	} else {
		/* window is somewhere else: swap with top */
		LIST_SWAP(&d->tiled, d->selcli, d->tiled);
	}
}

inline static struct client *
get_head(struct desktop *d, struct client *c)
{
	if (c == NULL)
		return NULL;
	return c->floating ? d->floating : d->tiled;
}

inline static struct client *
get_last(struct desktop *d, struct client *c)
{
	struct client *head;

	head = get_head(d, c);
	if (head == NULL)
		return NULL;
	return head->prev;
}

inline static struct client *
get_neighbour(struct client *c, enum list_direction dir)
{
	return c == NULL ? NULL : (dir == PREV) ? c->prev : c->next;
}
