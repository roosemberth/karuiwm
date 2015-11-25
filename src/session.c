#include "session.h"
#include "util.h"
#include "list.h"
#include <X11/Xlib.h>

static int scan_windows(struct session *s);

static int
scan_windows(struct session *s)
{
	Window *wins;
	Window win;
	int unsigned i, nwins;
	struct client *c;

	if (!XQueryTree(karuiwm.dpy, karuiwm.root, &win, &win, &wins, &nwins)) {
		WARN("XQueryTree() failed");
		return -1;
	}
	/* TODO restore session */
	for (i = 0; i < nwins; ++i) {
		c = client_new(wins[i]);
		if (c != NULL) {
			NOTICE("scanned window %lu", c->win);
			desktop_attach_client(s->workspaces->desktops, c);
		}
	}
	return 0;
}

void
session_attach_workspace(struct session *s, struct workspace *ws)
{
	LIST_APPEND(&s->workspaces, ws);
	++s->nws;
}

void
session_delete(struct session *s)
{
	struct workspace *ws;

	while (s->nws > 0) {
		ws = s->workspaces;
		session_detach_workspace(s, ws);
		workspace_delete(ws);
	}
	sfree(s);
}

void
session_detach_workspace(struct session *s, struct workspace *ws)
{
	if (s->nws == 0) {
		WARN("detaching workspace from empty session");
		return;
	}
	LIST_REMOVE(&s->workspaces, ws);
	--s->nws;
}

int
session_locate_window(struct session *s, struct client **c, Window w)
{
	int unsigned i;
	struct workspace *ws;

	for (i = 0, ws = s->workspaces; i < s->nws; ++i, ws = ws->next)
		if (workspace_locate_window(ws, c, w) == 0)
			return 0;
	return -1;
}

struct session *session_new(void)
{
	struct session *s;
	struct workspace *ws;

	/* create session */
	s = smalloc(sizeof(struct session), "session");
	s->nws = 0;
	s->workspaces = NULL;

	/* initial workspace (TODO configurable initial workspace name) */
	ws = workspace_new(karuiwm.env.APPNAME);
	session_attach_workspace(s, ws);

	/* scan for previously existing windows */
	scan_windows(s);

	return s;
}

int
session_save(struct session *s, char *sid, size_t sid_len)
{
	(void) s;
	(void) sid;
	(void) sid_len;

	/* TODO save session */

	return -1;
}
