#ifndef _KARUIWM_WORKSPACE_H
#define _KARUIWM_WORKSPACE_H

#include "desktop.h"
#include "karuiwm.h"

struct workspace {
	struct workspace *prev, *next;
	size_t nd;
	struct desktop *desktops;
	char name[BUFSIZ];
};

void workspace_attach_desktop(struct workspace *ws, struct desktop *d);
void workspace_delete(struct workspace *ws);
void workspace_detach_desktop(struct workspace *ws, struct desktop *d);
struct desktop *workspace_locate_desktop(struct workspace *ws,
                                         int posx, int posy);
int workspace_locate_window(struct workspace *ws, struct client **c, Window win);
void workspace_locate_free_slot(struct workspace *ws, int *posx, int *posy);
struct workspace *workspace_new(char const *name);

#endif /* ndef _KARUIWM_WORKSPACE_H */
