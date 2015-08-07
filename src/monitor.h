#ifndef _KARUIWM_MONITOR_H
#define _KARUIWM_MONITOR_H

#include "desktop.h"
#include "focus.h"
#include <stdbool.h>

struct monitor {
	struct monitor *prev, *next; /* see list.h */
	int x, y;
	int unsigned w, h;
	struct desktop *seldt;
	struct focus *focus;
	int unsigned index;
};

int unsigned monitor_client_intersect(struct monitor *m, struct client *c);
void monitor_delete(struct monitor *m);
void monitor_focus(struct monitor *m, bool focus);
struct monitor *monitor_new(struct focus *f, struct desktop *d, int x, int y,
                            int unsigned w, int unsigned h);
void monitor_show(struct monitor *m, struct desktop *d);
void monitor_step_desktop(struct monitor *m, enum direction dir);
void monitor_update_geometry(struct monitor *m,
                             int x, int y, int unsigned w, int unsigned h);

#endif /* ndef _KARUIWM_MONITOR_H */
