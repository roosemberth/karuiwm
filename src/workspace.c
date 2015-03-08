#include "workspace.h"
#include "util.h"
#include <string.h>

void
workspace_attach_desktop(struct workspace *ws, struct desktop *d)
{
	++ws->nd;
	ws->desktops = srealloc(ws->desktops, ws->nd*sizeof(d), "desktop list");
	ws->desktops[ws->nd - 1] = d;
}

void
workspace_delete(struct workspace *ws)
{
	struct desktop *d;

	while (ws->nd > 0) {
		d = ws->desktops[0];
		workspace_detach_desktop(ws, d);
		desktop_delete(d);
	}
	free(ws);
}

void
workspace_detach_desktop(struct workspace *ws, struct desktop *d)
{
	int unsigned i;

	for (i = 0; i < ws->nd && ws->desktops[i] != d; ++i);
	if (i == ws->nd) {
		WARN("attempt to detach non-existing desktop");
		return;
	}
	for (; i < ws->nd - 1; ++i)
		ws->desktops[i] = ws->desktops[i + 1];
	--ws->nd;
	ws->desktops = srealloc(ws->desktops, ws->nd*sizeof(d), "desktop list");
	if (ws->seldt == d)
		ws->seldt = ws->nd > 0 ? ws->desktops[0] : NULL;
}

struct desktop *
workspace_locate_desktop_xy(struct workspace *ws, int posx, int posy)
{
	int unsigned i;
	struct desktop *d;

	for (i = 0; i < ws->nd; ++i) {
		d = ws->desktops[i];
		if (d->posx == posx && d->posy == posy)
			return d;
	}
	return NULL;
}

struct workspace *
workspace_new(char const *name)
{
	struct workspace *ws;

	ws = smalloc(sizeof(struct workspace), "workspace");
	strncpy(ws->name, name, BUFSIZ);
	ws->name[BUFSIZ - 1] = '\0';
	ws->nd = 0;
	ws->desktops = NULL;
	ws->seldt = NULL;
	workspace_attach_desktop(ws, desktop_new(0, 0)); /* TODO */
	ws->seldt = ws->desktops[0];
	return ws;
}

void
workspace_update_focus(struct workspace *ws)
{
	if (ws->nd > 0)
		desktop_update_focus(ws->seldt);
}

void
workspace_step_desktop(struct workspace *ws, enum direction dir)
{
	struct desktop *src, *dst;
	int posx = ws->seldt->posx, posy = ws->seldt->posy;

	/* locate source and destination desktops */
	src = ws->seldt;
	switch (dir) {
	case LEFT:  --posx; break;
	case RIGHT: ++posx; break;
	case UP:    --posy; break;
	case DOWN:  ++posy; break;
	default: WARN("invalid direction for desktop step: %d", dir);
	}
	dst = workspace_locate_desktop_xy(ws, posx, posy);

	/* switch to destination */
	if (dst == NULL) {
		dst = desktop_new(posx, posy);
		workspace_attach_desktop(ws, dst);
	}
	ws->seldt = dst;
	/* TODO geometry */
	desktop_update_geometry(dst, 0, 0,
	                        (int unsigned) DisplayWidth(kwm.dpy, screen),
	                        (int unsigned) DisplayHeight(kwm.dpy, screen));
	desktop_show(dst);
	desktop_update_focus(dst);

	/* switch from source */
	desktop_hide(src);
	if (src == NULL) {
		src = ws->seldt;
		workspace_detach_desktop(ws, src);
		desktop_delete(src);
	} else {
		desktop_hide(src);
	}

}
