#include "karuiwm/karuiwm.h"
#include "karuiwm/desktop.h"
#include "karuiwm/util.h"
#include "karuiwm/cursor.h"
#include "karuiwm/api.h"

#include <unistd.h>
#include <string.h>
#include <errno.h>

int init(void);
void term(void);
static void action_killclient(union argument *arg);
static void action_mousemove(union argument *arg);
static void action_mouseresize(union argument *arg);
static void action_restart(union argument *arg);
static void action_setlayout(union argument *arg);
static void action_setmfact(union argument *arg);
static void action_setnmaster(union argument *arg);
static void action_shiftclient(union argument *arg);
static void action_spawn(union argument *arg);
static void action_stepclient(union argument *arg);
static void action_stepdesktop(union argument *arg);
static void action_stop(union argument *arg);
static void action_togglefloat(union argument *arg);
static void action_zoom(union argument *arg);
static void layout_monocle(struct client *clients, size_t nc,
                           size_t nmaster, float mfact,
                           int sx, int sy, int unsigned sw, int unsigned sh);
static void layout_rstack(struct client *clients, size_t nc,
                          size_t nmaster, float mfact,
                          int sx, int sy, int unsigned sw, int unsigned sh);
static void mouse_move(struct client *c, int mx, int my);
static void mouse_moveresize(struct client *c, void (*mh)(struct client *, int, int));
static void mouse_resize(struct client *c, int mx, int my);

int
init(void)
{
	api_add_action(action_new("core.killclient",  action_killclient,  ARGTYPE_NONE));
	api_add_action(action_new("core.mousemove",   action_mousemove,   ARGTYPE_MOUSE));
	api_add_action(action_new("core.mouseresize", action_mouseresize, ARGTYPE_MOUSE));
	api_add_action(action_new("core.restart",     action_restart,     ARGTYPE_NONE));
	api_add_action(action_new("core.setlayout",   action_setlayout,   ARGTYPE_LAYOUT));
	api_add_action(action_new("core.setmfact",    action_setmfact,    ARGTYPE_FLOATING));
	api_add_action(action_new("core.setnmaster",  action_setnmaster,  ARGTYPE_INTEGER));
	api_add_action(action_new("core.shiftclient", action_shiftclient, ARGTYPE_LIST_DIRECTION));
	api_add_action(action_new("core.spawn",       action_spawn,       ARGTYPE_STRING));
	api_add_action(action_new("core.stepclient",  action_stepclient,  ARGTYPE_LIST_DIRECTION));
	api_add_action(action_new("core.stepdesktop", action_stepdesktop, ARGTYPE_DIRECTION));
	api_add_action(action_new("core.stop",        action_stop,        ARGTYPE_NONE));
	api_add_action(action_new("core.togglefloat", action_togglefloat, ARGTYPE_NONE));
	api_add_action(action_new("core.zoom",        action_zoom,        ARGTYPE_NONE));
	api_add_layout(layout_new("core.monocle",     layout_monocle));
	api_add_layout(layout_new("core.rstack",      layout_rstack));
	return 0;
}

void
term(void)
{
	/* API cleans itself, no need to remove actions */
}

static void
action_killclient(union argument *arg)
{
	(void) arg;

	desktop_kill_client(karuiwm.focus->selmon->seldt);
}

static void
action_mousemove(union argument *arg)
{
	struct client *c;
	Window win = *((Window *) arg->v);

	if (session_locate_window(karuiwm.session, &c, win) < 0) {
		WARN("attempt to mouse-move unhandled window %lu", win);
		return;
	}
	mouse_moveresize(karuiwm.focus->selmon->seldt->selcli, mouse_move);
}

static void
action_mouseresize(union argument *arg)
{
	struct client *c;
	Window win = *((Window *) arg->v);

	if (session_locate_window(karuiwm.session, &c, win) < 0) {
		WARN("attempt to mouse-resize unhandled window %lu", win);
		return;
	}
	mouse_moveresize(c, mouse_resize);
}

static void
action_restart(union argument *arg)
{
	action_stop(arg);
	karuiwm.restarting = true;
}

static void
action_setmfact(union argument *arg)
{
	struct desktop *d = karuiwm.focus->selmon->seldt;

	desktop_set_mfact(d, d->mfact + arg->f);
	desktop_arrange(d);
}

static void
action_setnmaster(union argument *arg)
{
	struct desktop *d = karuiwm.focus->selmon->seldt;

	if (arg->i < 0 && (size_t) (-arg->i) > d->nmaster)
		return;
	desktop_set_nmaster(d, (size_t) ((int signed) d->nmaster + arg->i));
	desktop_arrange(d);
}

static void
action_shiftclient(union argument *arg)
{
	struct desktop *d = karuiwm.focus->selmon->seldt;

	if (d->selcli == NULL)
		return;
	(void) desktop_shift_client(d, arg->i);
	desktop_arrange(d);
}

static void
action_spawn(union argument *arg)
{
	char const *cmd = (char const *) arg->v;

	pid_t pid = fork();
	if (pid == 0) {
		execl("/bin/sh", "sh", "-c", cmd, (char *) NULL);
		ERROR("execl(%s) failed: %s", cmd, strerror(errno));
	}
	if (pid < 0)
		ERROR("fork() failed with code %d", pid);
}

static void
action_stepclient(union argument *arg)
{
	struct desktop *d = karuiwm.focus->selmon->seldt;

	desktop_step_client(d, arg->i);
	desktop_arrange(d);
}

static void
action_stepdesktop(union argument *arg)
{
	monitor_step_desktop(karuiwm.focus->selmon, arg->i);
}

static void
action_setlayout(union argument *arg)
{
	struct desktop *d = karuiwm.focus->selmon->seldt;
	struct layout *layout = (struct layout *) arg->v;

	desktop_set_layout(d, layout);
	desktop_arrange(d);
}

static void
action_stop(union argument *arg)
{
	(void) arg;

	karuiwm.running = false;
	karuiwm.restarting = false;
}

static void
action_togglefloat(union argument *arg)
{
	struct desktop *d = karuiwm.focus->selmon->seldt;
	(void) arg;

	desktop_float_client(d, d->selcli, !d->selcli->floating);
	desktop_arrange(d);
}

static void
action_zoom(union argument *arg)
{
	struct desktop *d = karuiwm.focus->selmon->seldt;
	(void) arg;

	desktop_zoom(d);
	desktop_arrange(d);
}

static void
layout_monocle(struct client *clients, size_t nc, size_t nmaster, float mfact,
               int sx, int sy, int unsigned sw, int unsigned sh)
{
	int unsigned i;
	struct client *c;
	(void) nmaster;
	(void) mfact;

	for (i = 0, c = clients; i < nc; ++i, c = c->next)
		client_moveresize(c, sx, sy,
		                  sw - 2*c->border, sh - 2*c->border);
}

static void
layout_rstack(struct client *clients, size_t nc, size_t nmaster, float mfact,
              int sx, int sy, int unsigned sw, int unsigned sh)
{
	int unsigned i = 0, w, h;
	struct client *c = clients;
	int x, y;

	/* master area */
	if (nmaster > 0) {
		x = 0;
		w = nmaster == nc ? sw : (int unsigned) (mfact * (float) sw);
		h = sh / (int unsigned) nmaster;
		for (; i < nmaster; ++i, c = c->next) {
			y = (int signed) (i*h);
			client_moveresize(c, sx + x, sy + y,
			                  w - 2*c->border, h - 2*c->border);
		}
	}

	/* stack area */
	if (nc > nmaster) {
		x = i > 0 ? (int) (mfact * (float) sw) : 0;
		w = i > 0 ? sw - (int unsigned) x : sw;
		h = sh / (int unsigned) (nc - nmaster);
		for (; i < nc; ++i, c = c->next) {
			y = (int signed) ((i - nmaster)*h);
			client_moveresize(c, sx + x, sy + y,
			                  w - 2*c->border, h - 2*c->border);
		}
	}
}

static void
mouse_move(struct client *c, int mx, int my)
{
	XEvent ev;
	long evmask = MOUSEMASK | ExposureMask | SubstructureRedirectMask;
	int dx, dy;
	int cx = c->floatx;
	int cy = c->floaty;

	if (cursor_set_type(karuiwm.cursor, CURSOR_MOVE) < 0)
		WARN("could not change cursor appearance to moving");
	do {
		XMaskEvent(karuiwm.dpy, evmask, &ev);
		switch (ev.type) {
		case ButtonRelease:
			break;
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handle_event(&ev);
			break;
		case MotionNotify:
			dx = ev.xmotion.x - mx;
			dy = ev.xmotion.y - my;
			cx = cx + dx;
			cy = cy + dy;
			mx = ev.xmotion.x;
			my = ev.xmotion.y;
			client_move(c, cx, cy);
			break;
		default:
			WARN("unhandled event %d", ev.type);
		}
	} while (ev.type != ButtonRelease);
	if (cursor_set_type(karuiwm.cursor, CURSOR_NORMAL) < 0)
		WARN("could not reset cursor appearance");
	focus_associate_client(karuiwm.focus, c);
}

static void
mouse_moveresize(struct client *c, void (*mh)(struct client *c, int, int))
{
	int mx, my;

	if (c == NULL || c->state != STATE_NORMAL)
		return;

	if (cursor_get_pos(karuiwm.cursor, &mx, &my) < 0) {
		WARN("could not get cursor position");
		return;
	}
	focus_monitor_by_mouse(karuiwm.focus, mx, my);

	/* fix client floating position, float */
	if (c->floatx + (int signed) c->floatw < mx)
		c->floatx = mx - (int signed) c->floatw + 1;
	if (c->floatx > mx)
		c->floatx = mx;
	if (c->floaty + (int signed) c->floath < my)
		c->floaty = my - (int signed) c->floath + 1;
	if (c->floaty > my)
		c->floaty = my;
	desktop_float_client(karuiwm.focus->selmon->seldt, c, true);
	desktop_arrange(karuiwm.focus->selmon->seldt);

	/* handle events */
	mh(c, mx, my);
}

static void
mouse_resize(struct client *c, int mx, int my)
{
	XEvent ev;
	long evmask = MOUSEMASK | ExposureMask | SubstructureRedirectMask;
	int dx, dy;
	bool left, right, top, bottom;
	int cx = c->floatx;
	int cy = c->floaty;
	int unsigned cw = c->floatw;
	int unsigned ch = c->floath;

	/* determine area, set cursor appearance */
	top = my - cy < (int signed) ch / 3;
	bottom = my - cy > 2 * (int signed) ch / 3;
	left = mx - cx < (int signed) cw / 3;
	right = mx - cx > 2 * (int signed) cw / 3;
	if (!top && !bottom && !left && !right)
		return;
	if (cursor_set_type(karuiwm.cursor,
	                    top && left     ? CURSOR_RESIZE_TOP_LEFT     :
	                    top && right    ? CURSOR_RESIZE_TOP_RIGHT    :
	                    bottom && left  ? CURSOR_RESIZE_BOTTOM_LEFT  :
	                    bottom && right ? CURSOR_RESIZE_BOTTOM_RIGHT :
	                    left            ? CURSOR_RESIZE_LEFT         :
	                    right           ? CURSOR_RESIZE_RIGHT        :
	                    top             ? CURSOR_RESIZE_TOP          :
	                    bottom          ? CURSOR_RESIZE_BOTTOM       :
	                    /* ignore */      CURSOR_NORMAL) < 0)
	        WARN("could not change cursor appearance to resizing");
	do {
		XMaskEvent(karuiwm.dpy, evmask, &ev);
		switch (ev.type) {
		case ButtonRelease:
			break;
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handle_event(&ev);
			break;
		case MotionNotify:
			dx = ev.xmotion.x - mx;
			dy = ev.xmotion.y - my;

			/* incremental size */
			if (c->incw > 0)
				dx = dx / (int signed) c->incw
				        * (int signed) c->incw;
			if (c->inch > 0)
				dy = dy / (int signed) c->inch
				        * (int signed) c->inch;

			/* minimum size (x) */
			if (left
			&& (int signed) cw - dx >= (int signed) c->minw) {
				cx += dx;
				cw = (int unsigned) ((int signed) cw - dx);
			} else if (right
			&& (int signed) cw + dx >= (int signed) c->minw) {
				cw = (int unsigned) ((int signed) cw + dx);
			} else {
				dx = 0;
			}

			/* minimum size (y) */
			if (top
			&& (int signed) ch - dy >= (int signed) c->minh) {
				cy += dy;
				ch = (int unsigned) ((int signed) ch - dy);
			} else if (bottom
			&& (int signed) ch + dy >= (int signed) c->minh) {
				ch = (int unsigned) ((int signed) ch + dy);
			} else {
				dy = 0;
			}

			/* update cursor position, resize */
			mx += dx;
			my += dy;
			client_moveresize(c, cx, cy, cw, ch);
			break;
		default:
			WARN("unhandled event %d", ev.type);
		}
	} while (ev.type != ButtonRelease);
	if (cursor_set_type(karuiwm.cursor, CURSOR_NORMAL) < 0)
		WARN("could not reset cursor appearance");
	focus_associate_client(karuiwm.focus, c);
}
