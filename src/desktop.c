#include "desktop.h"
#include "util.h"

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
