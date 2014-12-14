#include "karuiwm.h"
#include "workspace.h"
#include <stdlib.h>
#include <string.h>

struct workspace *
workspace_init(int x, int y)
{
	struct workspace *ws;

	if (locatews(&ws, NULL, x, y, NULL)) {
		return ws;
	}
	ws = malloc(sizeof(struct workspace));
	if (!ws) {
		ERROR("could not allocate %zu bytes for workspace",
		      sizeof(struct workspace));
		exit(EXIT_FAILURE);
	}
	ws->x = x;
	ws->y = y;
	ws->mfact = 0.5;
	ws->nmaster = 1;
	ws->ilayout = 0;
	ws->dirty = false;
	workspace_rename(ws, NULL);
	if (wsm.active) {
		initwsmbox(ws);
	} else {
		ws->wsmbox = ws->wsmpm = 0;
	}
	return ws;
}

void
workspace_rename(struct workspace *ws, char const *name)
{
	if (name == NULL || strlen(name) == 0) {
		sprintf(ws->name, "*%p", (void *) ws);
		return;
	}
	if (name[0] != '*' && !locatews(NULL, NULL, 0, 0, name)) {
		strncpy(ws->name, name, 256);
	}
	if (strcmp(name, "chat") == 0) { /* TODO rules list */
		/* TODO layout pointer instead of index */
		ws->ilayout = 4;
		if (ws->mon != NULL) {
			arrange(ws->mon);
		}
	}
}
