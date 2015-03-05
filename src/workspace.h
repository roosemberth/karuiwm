#ifndef _WORKSPACE_H
#define _WORKSPACE_H

#include "desktop.h"

struct workspace {
	size_t nd;
	struct desktop **desktops, *seldt;
	char name[BUFSIZ];
};

void workspace_attach_desktop(struct workspace *ws, struct desktop *d);
void workspace_detach_desktop(struct workspace *ws, struct desktop *d);
struct workspace *workspace_new(char const *name);
void workspace_delete(struct workspace *ws);

#endif /* ndef _WORKSPACE_H */
