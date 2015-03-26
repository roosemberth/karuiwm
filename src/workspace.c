#include "workspace.h"
#include "util.h"
#include "list.h"
#include <string.h>

void
workspace_attach_desktop(struct workspace *ws, struct desktop *d)
{
	workspace_locate_free_slot(ws, &d->posx, &d->posy);
	LIST_APPEND(&ws->desktops, d);
	++ws->nd;
	d->workspace = ws;
}

void
workspace_delete(struct workspace *ws)
{
	struct desktop *d;

	while (ws->nd > 0) {
		d = ws->desktops;
		workspace_detach_desktop(ws, d);
		desktop_delete(d);
	}
	free(ws);
}

void
workspace_detach_desktop(struct workspace *ws, struct desktop *d)
{
	LIST_REMOVE(&ws->desktops, d);
	--ws->nd;
}

struct desktop *
workspace_locate_desktop(struct workspace *ws, int posx, int posy)
{
	int unsigned i;
	struct desktop *d;

	for (i = 0, d = ws->desktops; i < ws->nd; ++i, d = d->next)
		if (d->posx == posx && d->posy == posy)
			return d;
	return NULL;
}

void
workspace_locate_free_slot(struct workspace *ws, int *posx, int *posy)
{
	int size;

	/* TODO ugly as hell and inefficient way for searching empty spot */
	for (size = 0;; ++size)
		for (*posx = -size; *posx <= size; ++*posx)
			for (*posy = -size; *posy <= size; ++*posy)
				if (workspace_locate_desktop(ws, *posx, *posy) == NULL)
					return;
}

int
workspace_locate_window(struct workspace *ws, struct client **c, Window win)
{
	int unsigned i;
	struct desktop *d;

	for (i = 0, d = ws->desktops; i < ws->nd; ++i, d = d->next)
		if (desktop_locate_window(d, c, win) == 0)
			return 0;
	return -1;
}

struct workspace *
workspace_new(char const *name)
{
	struct workspace *ws;
	struct desktop *d;

	/* create workspace */
	ws = smalloc(sizeof(struct workspace), "workspace");
	strncpy(ws->name, name, BUFSIZ);
	ws->name[BUFSIZ - 1] = '\0';
	ws->nd = 0;
	ws->desktops = NULL;

	/* initial desktop */
	d = desktop_new();
	workspace_attach_desktop(ws, d);
	return ws;
}
