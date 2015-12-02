#ifndef _KARUIWM_FOCUS_H
#define _KARUIWM_FOCUS_H

#include "monitor.h"
#include "session.h"

struct focus {
	size_t nmon;
	struct monitor *monitors, *selmon;
	struct session *session;
};

void focus_associate_client(struct focus *f, struct client *c);
void focus_attach_monitor(struct focus *f, struct monitor *m);
void focus_delete(struct focus *f);
void focus_detach_monitor(struct focus *f, struct monitor *m);
void focus_monitor_by_mouse(struct focus *f, int x, int y);
void focus_monitor(struct focus *f, struct monitor *m);
struct focus *focus_new(struct session *s);
void focus_scan_monitors(struct focus *f);
void focus_step_monitor(struct focus *f, enum direction dir);

#endif /* ndef _KARUIWM_FOCUS_H */
