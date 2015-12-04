#include "core.h"
#include "argument.h"
#include "karuiwm.h"
#include "desktop.h"
#include "util.h"
#include "cursor.h"
#include "api.h"
#include <unistd.h>
#include <string.h>
#include <errno.h>

static void action_killclient(union argument *arg);
static void action_mousemove(union argument *arg);
static void action_mouseresize(union argument *arg);
static void action_restart(union argument *arg);
static void action_setmfact(union argument *arg);
static void action_setnmaster(union argument *arg);
static void action_shiftclient(union argument *arg);
static void action_spawn(union argument *arg);
static void action_stepclient(union argument *arg);
static void action_stepdesktop(union argument *arg);
static void action_steplayout(union argument *arg);
static void action_stop(union argument *arg);
static void action_togglefloat(union argument *arg);
static void action_zoom(union argument *arg);
static void mouse_move(struct client *c, int mx, int my);
static void mouse_moveresize(struct client *c, void (*mh)(struct client *, int, int));
static void mouse_resize(struct client *c, int mx, int my);

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

	//EVENT("movemouse(%lu)", c->win);

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

	//EVENT("resizemouse(%lu)", c->win);

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
		FATAL("execl(%s) failed: %s", cmd, strerror(errno));
	}
	if (pid < 0)
		WARN("fork() failed with code %d", pid);
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
action_steplayout(union argument *arg)
{
	struct desktop *d = karuiwm.focus->selmon->seldt;

	desktop_step_layout(d, arg->i);
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

int
core_init(void)
{
	api_add_action(action_new("killclient",  action_killclient, ARGTYPE_NONE));
	api_add_action(action_new("mousemove",   action_mousemove,  ARGTYPE_NONE));
	api_add_action(action_new("mouseresize", action_mouseresize,ARGTYPE_NONE));
	api_add_action(action_new("restart",     action_restart,    ARGTYPE_NONE));
	api_add_action(action_new("setmfact",    action_setmfact,   ARGTYPE_FLOATING));
	api_add_action(action_new("setnmaster",  action_setnmaster, ARGTYPE_INTEGER));
	api_add_action(action_new("shiftclient", action_shiftclient,ARGTYPE_LIST_DIRECTION));
	api_add_action(action_new("spawn",       action_spawn,      ARGTYPE_STRING));
	api_add_action(action_new("stepclient",  action_stepclient, ARGTYPE_LIST_DIRECTION));
	api_add_action(action_new("stepdesktop", action_stepdesktop,ARGTYPE_DIRECTION));
	api_add_action(action_new("steplayout",  action_steplayout, ARGTYPE_LIST_DIRECTION));
	api_add_action(action_new("stop",        action_stop,       ARGTYPE_NONE));
	api_add_action(action_new("togglefloat", action_togglefloat,ARGTYPE_NONE));
	api_add_action(action_new("zoom",        action_zoom,       ARGTYPE_NONE));
	return 0;
}

void
core_term(void)
{
	/* empty */
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
			handle[ev.type](&ev);
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
			handle[ev.type](&ev);
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
