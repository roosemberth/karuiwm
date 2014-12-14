#ifndef _WORKSPACE_H
#define _WORKSPACE_H

#include "client.h"
#include <X11/Xlib.h>
#include <stdbool.h>

struct workspace {
	struct client **clients, *selcli;
	size_t nc, nmaster;
	Window wsmbox;
	Pixmap wsmpm;
	float mfact;
	char name[256];
	int x, y;
	int ilayout;
	bool dirty;
	struct monitor *mon;
};

struct workspace *workspace_init(int x, int y);
void workspace_rename(struct workspace *ws, char const *name);

#endif /* _WORKSPACE_H */
