#include "desktop.h"
#include "util.h"
#include "layout.h"

inline static struct client *get_head(struct desktop *d, struct client *c);
inline static struct client *get_last(struct desktop *d, struct client *c);
inline static struct client *get_neighbour(struct client *c, int dir);
static void swap(struct client **list, struct client *c1, struct client *c2);

/* Propagate the client arrangement to the X server by applying the selected
 * layout and enforcing the window stack.
 */
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
		/* TODO modular layout management */
		rstack(d->tiled, d->nt, MIN(d->nmaster, d->nt), d->mfact,
		       d->x, d->y, d->w, d->h);
		/* TODO focused window on top */
		for (i = 0, c = d->tiled; i < d->nt; ++i, c = c->next)
			if (c->state != STATE_FULLSCREEN)
				stack[is++] = c->win;
	}
	XRestackWindows(kwm.dpy, stack, (int signed) (d->nt + d->nf));
	desktop_set_clientmask(d, CLIENTMASK);
}

/* Attach a client to a desktop.
 */
void
desktop_attach_client(struct desktop *d, struct client *c)
{
	struct client **head;
	size_t *nc;

	/* determine lists */
	if (c->floating) {
		nc = &d->nf;
		head = &d->floating;
	} else {
		nc = &d->nt;
		head = &d->tiled;
	}

	/* other elements? */
	if (*nc == 0) {
		c->next = c->prev = c;
	} else {
		c->next = (*head);
		c->prev = (*head)->prev;
		(*head)->prev->next = c;
		(*head)->prev = c;
	}

	/* new head? */
	if (*nc == 0 || c->floating)
		*head = c;

	/* update */
	++(*nc);
	d->selcli = c;
}

/* Determine if a client is on a given desktop, and locate the client's position
 * in the desktop's client list.
 */
bool
desktop_contains_client(struct desktop *d, struct client *c)
{
	struct client *it;
	int unsigned i;

	if (c == NULL)
		return false;
	for (i = 0, it = d->tiled; i < d->nt; ++i, it = it->next)
		if (it == c)
			return true;
	for (i = 0, it = d->floating; i < d->nf; ++i, it = it->next)
		if (it == c)
			return true;
	return false;
}

/* Properly delete a desktop and all its contained elements.
 */
void
desktop_delete(struct desktop *d)
{
	struct client *c;

	if (d->tiled != NULL || d->floating != NULL) {
		WARN("deleting desktop that still contains clients");
		while (d->tiled != NULL) {
			c = d->tiled;
			desktop_detach_client(d, c);
			client_delete(c);
		}
		while (d->floating != NULL) {
			c = d->floating;
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
	struct client **head, *other;
	size_t *nc;

	/* determine lists */
	if (c->floating) {
		nc = &d->nf;
		head = &d->floating;
		other = d->tiled;
	} else {
		nc = &d->nt;
		head = &d->tiled;
		other = d->floating;
	}

	if (*nc == 0) {
		WARN("attempt to remove client from empty desktop");
		return;
	}
	if (c == NULL) {
		WARN("attempt to remote NULL client from desktop");
		return;
	}

	/* selected? */
	if (c == d->selcli) {
		/* other elements? */
		if (c->next != c)
			d->selcli = c->next;
		else
			d->selcli = other;
	}

	/* head? */
	if (c == *head)
		*head = c->next != c ? c->next : NULL;

	/* update */
	c->next->prev = c->prev;
	c->prev->next = c->next;
	c->prev = c->next = NULL;
	--(*nc);
}

/* Set the floating mode of a client in the context of a desktop.
 */
void
desktop_float_client(struct desktop *d, struct client *c, bool floating)
{
	if (c->state == STATE_FULLSCREEN)
		return;
	desktop_detach_client(d, c);
	client_set_floating(c, floating);
	desktop_attach_client(d, c);
}

/* Set the fullscreen mode of a client in the context of a desktop.
 */
void
desktop_fullscreen_client(struct desktop *d, struct client *c, bool fullscreen)
{
	(void) d;

	client_set_fullscreen(c, fullscreen);
}

/* Determine if a window is contained in a given desktop, and locate both the
 * corresponding client and the client's position in the desktop's client list.
 */
int
desktop_locate_window(struct desktop *d, Window win, struct client **c)
{
	struct client *it;
	int unsigned i;

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
	struct client *it;
	int unsigned i;

	for (i = 0, it = d->tiled; i < d->nt; ++i, it = it->next)
		XSelectInput(kwm.dpy, it->win, mask);
	for (i = 0, it = d->floating; i < d->nf; ++i, it = it->next)
		XSelectInput(kwm.dpy, it->win, mask);
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
void
desktop_shift_client(struct desktop *d, int dir)
{
	struct client *this, *other;

	this = d->selcli;
	if (this == NULL)
		return;
	other = get_neighbour(this, dir);
	swap(this->floating ? &d->floating : &d->tiled, this, other);
}

/* Move focus to next/previous client.
 */
void
desktop_step_focus(struct desktop *d, int dir)
{
	if (d->selcli == NULL || d->selcli->state == STATE_FULLSCREEN)
		return;

	d->selcli = get_neighbour(d->selcli, dir);
	desktop_update_focus(d);
}

/* Set the X input focus to the currently selected client on a desktop.
 * TODO abstract root window as a client
 */
void
desktop_update_focus(struct desktop *d)
{
	struct client *c;
	int unsigned i;

	if (d->tiled == NULL && d->floating == NULL) {
		XSetInputFocus(kwm.dpy, kwm.root, RevertToPointerRoot,
		               CurrentTime);
		return;
	}
	for (i = 0, c = d->tiled; i < d->nt; ++i, c = c->next)
		client_set_focus(c, c == d->selcli);
	for (i = 0, c = d->floating; i < d->nf; ++i, c = c->next)
		client_set_focus(c, c == d->selcli);
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
	if (d->tiled == NULL || d->tiled->next == NULL)
		return;
	if (d->selcli->floating || d->selcli->state != STATE_NORMAL)
		return;

	if (d->selcli == d->tiled) {
		/* window is at the top: swap with next below */
		swap(&d->tiled, d->tiled, d->tiled->next);
		d->selcli = d->tiled;
		desktop_update_focus(d);
	} else {
		/* window is somewhere else: swap with top */
		swap(&d->tiled, d->selcli, d->tiled);
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
get_neighbour(struct client *c, int dir)
{
	return c == NULL ? NULL : dir < 0 ? c->prev : c->next;
}

static void
swap(struct client **head, struct client *c1, struct client *c2)
{
	struct client *prev1, *next1, *prev2, *next2;

	if (c1 == NULL || c2 == NULL)
		return;

	prev1 = c1->prev;
	next1 = c1->next;
	prev2 = c2->prev;
	next2 = c2->next;

	if (c1 == c2) {
		/* same node */
	} else if (c1->next == c2 && c2->next == c1) {
		/* only two nodes in list */
	} else if (c1->next == c2) {
		/* next to each other */
		prev1->next = c2;
		c2->next = c1;
		c1->next = next2;
		next2->prev = c1;
		c1->prev = c2;
		c2->prev = prev1;
	} else if (c2->next == c1) {
		/* next to each other (alt.) */
		swap(head, c2, c1);
	} else {
		/* general case */
		prev1->next = c2;
		c2->next = next1;
		next1->prev = c2;
		c2->prev = prev1;
		prev2->next = c1;
		c1->next = next2;
		next2->prev = c1;
		c1->prev = prev2;
	}

	/* fix list head */
	if (*head == c1)
		*head = c2;
	else if (*head == c2)
		*head = c1;
}
