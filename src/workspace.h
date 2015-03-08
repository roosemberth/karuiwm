#ifndef _WORKSPACE_H
#define _WORKSPACE_H

#include "desktop.h"
#include "karuiwm.h"

struct workspace {
	size_t nd;
	struct desktop **desktops, *seldt;
	char name[BUFSIZ];
};

void workspace_attach_desktop(struct workspace *ws, struct desktop *d);
void workspace_delete(struct workspace *ws);
void workspace_detach_desktop(struct workspace *ws, struct desktop *d);
struct desktop *workspace_locate_desktop_xy(struct workspace *ws,
                                            int posx, int posy);
struct workspace *workspace_new(char const *name);
void workspace_update_focus(struct workspace *ws);
void workspace_step_desktop(struct workspace *ws, enum direction dir);

#endif /* ndef _WORKSPACE_H */
