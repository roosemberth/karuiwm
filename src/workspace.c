#include "workspace.h"
#include "util.h"
#include <string.h>

void
workspace_attach_desktop(struct workspace *ws, struct desktop *d)
{
	ws->desktops = srealloc(ws->desktops, ++ws->nd*sizeof(d),
	                        "desktop list");
	ws->desktops[ws->nd - 1] = d;
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
	ws->desktops = srealloc(ws->desktops, --ws->nd*sizeof(d),
	                        "desktop list");
	if (ws->seldt == d)
		ws->seldt = ws->nd > 0 ? ws->desktops[0] : NULL;
}

struct workspace *
workspace_new(char const *name)
{
	struct workspace *ws;

	ws = smalloc(sizeof(struct workspace), "workspace");
	strncpy(ws->name, name, BUFSIZ);
	ws->name[BUFSIZ - 1] = '\0';
	ws->seldt = NULL;
	ws->desktops = NULL;
	ws->nd = 0;
	return ws;
}

void
workspace_delete(struct workspace *ws)
{
	struct desktop *d;

	if (ws->nd > 0) {
		WARN("deleting workspace that still contains desktops");
		while (ws->nd > 0) {
			d = ws->desktops[0];
			workspace_detach_desktop(ws, d);
			desktop_delete(d);
		}
	}
	free(ws);
}
