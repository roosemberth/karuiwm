#include "focus.h"
#include "util.h"
#include "list.h"
#ifdef XINERAMA
# include <X11/extensions/Xinerama.h>
#endif

#ifdef XINERAMA
static struct desktop *get_free_desktop(struct focus *f); /* FIXME currently only used for Xinerama */
static void scan_xinerama(struct focus *f);
#endif

void
focus_associate_client(struct focus *f, struct client *c)
{
	struct monitor *m, *maxm = f->selmon;
	int unsigned i, area, maxarea = 0;

	for (i = 0, m = f->monitors; i < f->nmon; ++i, m = m->next) {
		area = monitor_client_intersect(m, c);
		if (area > maxarea) {
			maxarea = area;
			maxm = m;
		}
	}
	if (maxm != f->selmon)
		focus_monitor(f, maxm);
}

void
focus_attach_monitor(struct focus *f, struct monitor *m)
{
	LIST_APPEND(&f->monitors, m);
	++f->nmon;
	if (f->selmon == NULL)
		f->selmon = m;
}

void
focus_delete(struct focus *f)
{
	sfree(f);
}

void
focus_detach_monitor(struct focus *f, struct monitor *m)
{
	LIST_REMOVE(&f->monitors, m);
	if (f->selmon == m)
		f->selmon = f->nmon > 0 ? f->monitors : NULL;
}

void
focus_monitor_by_mouse(struct focus *f, int x, int y)
{
	int unsigned i;
	struct monitor *m;

	for (i = 0, m = f->monitors; i < f->nmon; ++i, m = m->next) {
		if (m->x <= x && m->x + (int signed) m->w > x
		&& m->y <= y && m->y + (int signed) m->h > y) {
			f->selmon = m;
			break;
		}
	}
}

void
focus_monitor(struct focus *f, struct monitor *m)
{
	monitor_focus(f->selmon, false);
	monitor_focus(m, true);
	f->selmon = m;
}

struct focus *
focus_new(struct session *s)
{
	struct focus *f;

	f = smalloc(sizeof(struct focus), "focus");
	f->nmon = 0;
	f->monitors = NULL;
	f->selmon = NULL;
	f->session = s;
	focus_scan_monitors(f);
	return f;
}

void
focus_scan_monitors(struct focus *f)
{
	struct monitor *m;

#ifdef XINERAMA
	if (XineramaIsActive(karuiwm.dpy)) {
		scan_xinerama(f);
		return;
	}
#endif

	/* trim monitor count to 1 */
	while (f->nmon > 1) {
		m = f->monitors->prev;
		focus_detach_monitor(f, m);
		monitor_delete(m);
	}
	if (f->nmon < 1) {
		m = monitor_new(f, f->session->workspaces->desktops, 0, 0,
	             (int unsigned) DisplayWidth(karuiwm.dpy, karuiwm.screen),
	             (int unsigned) DisplayHeight(karuiwm.dpy, karuiwm.screen));
		m->index = 0;
		focus_attach_monitor(f, m);
	}
	m = f->monitors;
	monitor_focus(m, true);
}

void
focus_step_monitor(struct focus *f, enum direction dir)
{
	f->selmon = dir < 0 ? f->selmon->prev : f->selmon->next;
	monitor_focus(f->selmon, true);
}

#ifdef XINERAMA
static struct desktop *
get_free_desktop(struct focus *f)
{
	int unsigned i;
	struct workspace *ws = f->session->workspaces;
	struct desktop *d;

	for (i = 0, d = ws->desktops; i < ws->nd; ++i, d = d->next)
		if (d->monitor == NULL)
			break;
	if (i == ws->nd) {
		d = desktop_new();
		workspace_attach_desktop(ws, d);
	}
	return d;
}
#endif

#ifdef XINERAMA
static void
scan_xinerama(struct focus *f)
{
	int dup;
	int unsigned raw_n, n, raw_i, i;
	struct monitor *m;
	XineramaScreenInfo *raw_info, *info;

	/* get screen information */
	raw_info = XineramaQueryScreens(karuiwm.dpy, (int *) &raw_n);

	/* de-duplicate screen information: O(n²) */
	info = scalloc(raw_n, sizeof(XineramaScreenInfo),
	               "Xinerama screen info");
	n = 0;
	for (raw_i = 0; raw_i < raw_n; ++raw_i) {
		dup = 0;
		for (i = 0; i < n && !dup; ++i) {
			dup = raw_info[raw_i].x_org == info[i].x_org &&
			      raw_info[raw_i].y_org == info[i].y_org &&
			      raw_info[raw_i].width == info[i].width &&
			      raw_info[raw_i].height == info[i].height;
		}
		if (!dup) {
			info[n].x_org = raw_info[raw_i].x_org;
			info[n].y_org = raw_info[raw_i].y_org;
			info[n].width = raw_info[raw_i].width;
			info[n].height = raw_info[raw_i].height;
			++n;
		}
	}
	XFree(raw_info);
	if (n == 0)
		FATAL("0 screens detected");

	/* trim monitor count to n */
	while (f->nmon > n) {
		m = f->monitors->prev;
		focus_detach_monitor(f, m);
		monitor_delete(m);
	}
	for (i = 0, m = f->monitors; i < n; ++i, m = m->next) {
		if (i >= f->nmon) {
			m = monitor_new(f, get_free_desktop(f),
			                info[i].x_org, info[i].y_org,
			                (int unsigned) info[i].width,
			                (int unsigned) info[i].height);
			focus_attach_monitor(f, m);
		} else {
			monitor_update_geometry(m, info[i].x_org, info[i].y_org,
			                        (int unsigned) info[i].width,
			                        (int unsigned) info[i].height);
		}
		m->index = i;
	}
	monitor_focus(f->monitors, true);

	XFree(info);
	XSync(karuiwm.dpy, karuiwm.screen);
}
#endif /* def XINERAMA */
