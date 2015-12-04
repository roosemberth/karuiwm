#include "karuiwm.h"
#include "desktop.h"
#include "util.h"
#include "monitor.h"

int unsigned
monitor_client_intersect(struct monitor *m, struct client *c)
{
	int left, right, top, bottom;

	left = MAX(m->x, c->x);
	right = MIN(m->x + (int signed) m->w, c->x + (int signed) c->w);
	top = MAX(m->y, c->y);
	bottom = MIN(m->y + (int signed) m->h, c->y + (int signed) c->h);

	return (int unsigned) (MAX(0, right - left) * MAX(0, bottom - top));
}

void
monitor_delete(struct monitor *m)
{
	desktop_show(m->seldt, NULL);
	free(m);
}

void
monitor_focus(struct monitor *m, bool focus)
{
	desktop_set_focus(m->seldt, focus);
}

struct monitor *
monitor_new(struct focus *f, struct desktop *d, int x, int y,
            int unsigned w, int unsigned h)
{
	struct monitor *m;

	m = smalloc(sizeof(struct monitor), "monitor");
	m->x = x;
	m->y = y;
	m->w = w;
	m->h = h;
	m->seldt = d;
	m->focus = f;
	desktop_show(d, m);
	return m;
}

void
monitor_show_desktop(struct monitor *m, struct desktop *d)
{
	struct desktop *old, *new;
	struct monitor *othermon;

	old = m->seldt;
	new = d;
	othermon = d->monitor;
	if (new == old)
		return;

	/* show new desktop */
	desktop_show(new, m);
	desktop_set_focus(new, m == m->focus->selmon);

	/* hide old desktop */
	if (old->nt + old->nf == 0) {
		workspace_detach_desktop(old->workspace, old);
		desktop_delete(old);
	} else {
		desktop_show(old, othermon);
	}

	m->seldt = new;
}

void
monitor_step_desktop(struct monitor *m, enum direction dir)
{
	struct desktop *src, *dst;
	struct workspace *ws;
	int posx = m->seldt->posx, posy = m->seldt->posy;

	src = m->seldt;
	ws = src->workspace;
	switch (dir) {
	case LEFT:  --posx; break;
	case RIGHT: ++posx; break;
	case UP:    --posy; break;
	case DOWN:  ++posy; break;
	default: WARN("invalid direction for desktop step: %d", dir);
	}
	dst = workspace_locate_desktop(ws, posx, posy);
	if (dst == NULL) {
		dst = desktop_new();
		workspace_attach_desktop(ws, dst);
		/* FIXME desktop positioning hack */
		dst->posx = posx;
		dst->posy = posy;
	}
	monitor_show_desktop(m, dst);
}

void
monitor_update_geometry(struct monitor *m,
                        int x, int y, int unsigned w, int unsigned h)
{
	m->x = x;
	m->y = y;
	m->w = w;
	m->h = h;
	desktop_arrange(m->seldt);
}
