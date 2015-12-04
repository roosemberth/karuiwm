#define _XOPEN_SOURCE 500

#include "karuiwm.h"
#include "client.h"
#include "desktop.h"
#include "workspace.h"
#include "session.h"
#include "monitor.h"
#include "focus.h"
#include "cursor.h"
#include "layout.h"
#include "config.h"
#include "util.h"
#include "list.h"
#include "argument.h"
#include "keybind.h"
#include "buttonbind.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xproto.h>

/* macros */
#define _INIT_ATOM(D, L, A) L[A] = XInternAtom(D, #A, False)
#define BUFSIZE 1024

/* functions */
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
static void check_restart(char **argv);
static void grabkeys(void);
static void handle_buttonpress(XEvent *xe);
static void handle_clientmessage(XEvent *xe);
static void handle_configurerequest(XEvent *xe);
static void handle_configurenotify(XEvent *xe);
static void handle_destroynotify(XEvent *xe);
static void handle_enternotify(XEvent *xe);
static void handle_expose(XEvent *xe);
static void handle_focusin(XEvent *xe);
static void handle_keypress(XEvent *xe);
static void handle_mappingnotify(XEvent *xe);
static void handle_maprequest(XEvent *xe);
static void handle_propertynotify(XEvent *xe);
static int handle_xerror(Display *dpy, XErrorEvent *xe);
static void init(void);
static void init_actions(void);
static void init_atoms(void);
static void mouse_move(struct client *c, int mx, int my);
static void mouse_moveresize(struct client *c, void (*mh)(struct client *, int, int));
static void mouse_resize(struct client *c, int mx, int my);
static void parse_args(int argc, char **argv);
static void run(void);
static void sigchld(int);
static void term(void);

/* event handlers, as array to allow O(1) access; numeric codes are in X.h */
static void (*handle[LASTEvent])(XEvent *) = {
	[ButtonPress]      = handle_buttonpress,      /* 4*/
	[ClientMessage]    = handle_clientmessage,    /*33*/
	[ConfigureNotify]  = handle_configurenotify,  /*22*/
	[ConfigureRequest] = handle_configurerequest, /*23*/
	[DestroyNotify]    = handle_destroynotify,    /*17*/
	[EnterNotify]      = handle_enternotify,      /* 7*/
	[Expose]           = handle_expose,           /*12*/
	[FocusIn]          = handle_focusin,          /* 9*/
	[KeyPress]         = handle_keypress,         /* 2*/
	[MapRequest]       = handle_maprequest,       /*20*/
	[MappingNotify]    = handle_mappingnotify,    /*34*/
	[PropertyNotify]   = handle_propertynotify,   /*28*/
};
static int (*xerrorxlib)(Display *dpy, XErrorEvent *xe);

/* implementation */
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

static void
check_restart(char **argv)
{
	if (karuiwm.restarting) {
		VERBOSE("restarting ...");
		execvp(argv[0], argv);
		ERROR("restart failed: %s", strerror(errno));
	} else {
		VERBOSE("shutting down ...");
	}
}

static void
grabkeys(void)
{
	int unsigned i;
	struct keybind *kb;

	XUngrabKey(karuiwm.dpy, AnyKey, AnyModifier, karuiwm.root);
	for (i = 0, kb = config.keybinds; i < config.nkeybinds;
	     ++i, kb = kb->next)
		XGrabKey(karuiwm.dpy, XKeysymToKeycode(karuiwm.dpy, kb->key),
		         kb->mod, karuiwm.root, True, GrabModeAsync,
		         GrabModeAsync);
}

static void
handle_buttonpress(XEvent *xe)
{
	int unsigned i;
	struct buttonbind *bb;
	XButtonEvent *e = &xe->xbutton;

	//EVENT("buttonpress(%lu)", e->window);

	/* TODO define actions for clicks on root window */
	if (e->window == karuiwm.root)
		return;

	for (i = 0, bb = config.buttonbinds; i < config.nbuttonbinds;
	     ++i, bb = bb->next) {
		if (bb->mod == e->state && bb->button == e->button) {
			bb->action->function(&((union argument) {.v = &e->window}));
			break;
		}
	}
}

static void
handle_clientmessage(XEvent *xe)
{
	bool fullscreen;
	struct client *c;
	XClientMessageEvent *e = &xe->xclient;

	//EVENT("clientmessage(%lu)", e->window);

	if (session_locate_window(karuiwm.session, &c, e->window) < 0)
		return;
	if (e->message_type != netatoms[_NET_WM_STATE]) {
		WARN("received client message for other than WM state");
		return;
	}
	if ((Atom) e->data.l[1] == netatoms[_NET_WM_STATE_FULLSCREEN]
	|| (Atom) e->data.l[2] == netatoms[_NET_WM_STATE_FULLSCREEN]) {
		/* _NET_WM_STATE_ADD || _NET_WM_STATE_TOGGLE */
		fullscreen = (Atom) e->data.l[0] == netatoms[_NET_WM_STATE_ADD] ||
			    ((Atom) e->data.l[0] == netatoms[_NET_WM_STATE_TOGGLE] &&
			     c->state != STATE_FULLSCREEN);
		desktop_fullscreen_client(c->desktop, c, fullscreen);
		desktop_arrange(c->desktop);
	}
}

static void
handle_configurenotify(XEvent *xe)
{
	XConfigureEvent *e = &xe->xconfigure;

	//EVENT("configurenotify(%lu)", e->window);

	if (e->window == karuiwm.root)
		focus_scan_monitors(karuiwm.focus);
}

static void
handle_configurerequest(XEvent *xe)
{
	XWindowChanges wc;
	XConfigureRequestEvent *e = &xe->xconfigurerequest;

	//EVENT("configurerequest(%lu)", e->window);

	/* TODO if dimensions match screen dimensions, fullscreen (mplayer) */

	wc.x = e->x;
	wc.y = e->y;
	wc.width = e->width;
	wc.height = e->height;
	wc.border_width = e->border_width;
	wc.sibling = e->above;
	wc.stack_mode = e->detail;
	XConfigureWindow(karuiwm.dpy, e->window, (int unsigned) e->value_mask,
	                 &wc);
}

static void
handle_destroynotify(XEvent *xe)
{
	struct client *c;
	struct desktop *d;
	XDestroyWindowEvent *e = &xe->xdestroywindow;
	bool was_transient;

	//EVENT("destroynotify(%lu)", e->window);

	if (session_locate_window(karuiwm.session, &c, e->window) < 0)
		return;

	was_transient = c->transient;
	d = c->desktop;
	desktop_detach_client(d, c);
	client_delete(c);
	desktop_arrange(d);
	if (d->monitor != NULL && !was_transient)
		desktop_update_focus(d);
}

static void
handle_enternotify(XEvent *xe)
{
	struct client *c;
	XEnterWindowEvent *e = &xe->xcrossing;

	//EVENT("enternotify(%lu)", e->window);

	if (session_locate_window(karuiwm.session, &c, e->window) < 0) {
		WARN("entering unhandled window %lu", e->window);
		return;
	}
	if (c->desktop->selcli == c && e->focus) {
		/* ignore event for windows that already have focus */
		return;
	}
	desktop_focus_client(c->desktop, c);
}

static void
handle_expose(XEvent *xe)
{
	XExposeEvent *e = &xe->xexpose;

	//EVENT("expose(%lu)", e->window);

	/* TODO handle expose events */
	(void) e;
}

static void
handle_focusin(XEvent *xe)
{
	XFocusInEvent *e = &xe->xfocus;
	struct desktop *seldt = karuiwm.focus->selmon->seldt;
	struct client *selcli = seldt->selcli;

	//EVENT("focusin(%lu)", e->window);

	if (e->window == karuiwm.root)
		return;
	if (selcli == NULL || e->window != selcli->win) {
		WARN("attempt to steal focus by window %lu (focus is on %lu)",
		     e->window, selcli == NULL ? 0 : selcli->win);
		desktop_focus_client(seldt, selcli);
	}
}

static void
handle_keypress(XEvent *xe)
{
	int unsigned i;
	struct keybind *kb;
	XKeyPressedEvent *e = &xe->xkey;

	//EVENT("keypress()");

	KeySym keysym = XLookupKeysym(e, 0);
	for (i = 0, kb = config.keybinds; i < config.nkeybinds;
	     ++i, kb = kb->next) {
		if (e->state == kb->mod && keysym == kb->key) {
			kb->action->function(&kb->arg);
			break;
		}
	}
}

static void
handle_mappingnotify(XEvent *xe)
{
	XMappingEvent *e = &xe->xmapping;

	//EVENT("mappingnotify(%lu)", e->window);

	/* TODO handle keyboard mapping changes */
	WARN("keyboard mapping not implemented (window %lu)", e->window);
}

static void
handle_maprequest(XEvent *xe)
{
	struct client *c;
	struct desktop *d;
	XMapRequestEvent *e = &xe->xmaprequest;

	//EVENT("maprequest(%lu)", e->window);

	/* window is being remapped */
	if (session_locate_window(karuiwm.session, &c, e->window) == 0) {
		XMapWindow(karuiwm.dpy, c->win);
		return;
	}

	c = client_new(e->window);
	if (c == NULL)
		return;

	/* fix floating dimensions */
	d = karuiwm.focus->selmon->seldt;
	desktop_attach_client(d, c);
	if (c->floating)
		client_moveresize(c, MAX(c->floatx, 0),
		                     MAX(c->floaty, 0),
		                     MIN(c->floatw, d->monitor->w),
		                     MIN(c->floath, d->monitor->h));
	desktop_arrange(d);
	XMapWindow(karuiwm.dpy, c->win);
	client_grab_buttons(c, config.nbuttonbinds, config.buttonbinds);
	desktop_focus_client(d, c);
}

static void
handle_propertynotify(XEvent *xe)
{
	XPropertyEvent *e = &xe->xproperty;
	struct client *c;

	//EVENT("propertynotify(%lu)", e->window);

	if (session_locate_window(karuiwm.session, &c, e->window) < 0)
		return;

	if (e->state == PropertyDelete) {
		/* TODO handle property deletion */
		WARN("property deletion not implemented (window %lu)", c->win);
		return;
	}

	switch (e->atom) {
	case XA_WM_TRANSIENT_FOR:
		DEBUG("transient property changed for window %lu", c->win);
		client_query_transient(c);
		desktop_arrange(c->desktop);
		break;
	case XA_WM_NORMAL_HINTS:
		//DEBUG("size hints changed for window %lu", c->win);
		client_query_sizehints(c);
		break;
	case XA_WM_HINTS:
		WARN("urgent hint not implemented (window %lu)", c->win);
		/* TODO implement urgent hint handling */
		break;
	}
	if (e->atom == XA_WM_NAME || e->atom == netatoms[_NET_WM_NAME])
		client_query_name(c);
	if (e->atom == netatoms[_NET_WM_WINDOW_TYPE]) {
		client_query_fullscreen(c);
		client_query_dialog(c);
		desktop_arrange(c->desktop);
	}
}

static int
handle_xerror(Display *dpy, XErrorEvent *ee)
{
	char es[256];
	bool ignore;

	XGetErrorText(dpy, ee->error_code, es, 256);
	ERROR("%s after request %d", es, ee->request_code);
	ignore = ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch);

	/* default error handler exits */
	return ignore ? 0 : xerrorxlib(dpy, ee);
}

static void
init(void)
{
	XSetWindowAttributes wa;

	/* environment */
	karuiwm.env.HOME = getenv("HOME");
	if (karuiwm.env.HOME == NULL)
		WARN("HOME is not set, user-specific modules cannot be loaded");

	/* connect to X */
	karuiwm.dpy = XOpenDisplay(NULL);
	if (karuiwm.dpy == NULL)
		FATAL("could not open X");

	/* errors, zombies, locale */
	xerrorxlib = XSetErrorHandler(handle_xerror);
	sigchld(0);
	if (setlocale(LC_ALL, "") == NULL)
		FATAL("could not set locale");
	if (!XSupportsLocale())
		FATAL("X does not support locale");

	/* X screen, root window, atoms */
	karuiwm.screen = DefaultScreen(karuiwm.dpy);
	karuiwm.root = RootWindow(karuiwm.dpy, karuiwm.screen);
	karuiwm.xfd = ConnectionNumber(karuiwm.dpy);
	karuiwm.cm = DefaultColormap(karuiwm.dpy, karuiwm.screen);
	init_atoms();

	/* events */
	wa.event_mask = SubstructureNotifyMask | SubstructureRedirectMask |
	                PropertyChangeMask | FocusChangeMask | ButtonPressMask |
	                KeyPressMask | PointerMotionMask | StructureNotifyMask;
	XChangeWindowAttributes(karuiwm.dpy, karuiwm.root, CWEventMask, &wa);

	/* actions */
	init_actions();

	/* user configuration */
	if (config_init() < 0)
		FATAL("could not initialise X resources");

	/* input (mouse, keyboard) */
	karuiwm.cursor = cursor_new();
	grabkeys();

	/* layouts, session, focus */
	layout_init();
	karuiwm.session = session_new();
	karuiwm.focus = focus_new(karuiwm.session);
}

static void
init_actions(void)
{
	actions = NULL;
	LIST_APPEND(&actions, action_new("killclient",  action_killclient, ARGTYPE_NONE));
	LIST_APPEND(&actions, action_new("mousemove",   action_mousemove,  ARGTYPE_NONE));
	LIST_APPEND(&actions, action_new("mouseresize", action_mouseresize,ARGTYPE_NONE));
	LIST_APPEND(&actions, action_new("restart",     action_restart,    ARGTYPE_NONE));
	LIST_APPEND(&actions, action_new("setmfact",    action_setmfact,   ARGTYPE_FLOATING));
	LIST_APPEND(&actions, action_new("setnmaster",  action_setnmaster, ARGTYPE_INTEGER));
	LIST_APPEND(&actions, action_new("shiftclient", action_shiftclient,ARGTYPE_LIST_DIRECTION));
	LIST_APPEND(&actions, action_new("spawn",       action_spawn,      ARGTYPE_STRING));
	LIST_APPEND(&actions, action_new("stepclient",  action_stepclient, ARGTYPE_LIST_DIRECTION));
	LIST_APPEND(&actions, action_new("stepdesktop", action_stepdesktop,ARGTYPE_DIRECTION));
	LIST_APPEND(&actions, action_new("steplayout",  action_steplayout, ARGTYPE_LIST_DIRECTION));
	LIST_APPEND(&actions, action_new("stop",        action_stop,       ARGTYPE_NONE));
	LIST_APPEND(&actions, action_new("togglefloat", action_togglefloat,ARGTYPE_NONE));
	LIST_APPEND(&actions, action_new("zoom",        action_zoom,       ARGTYPE_NONE));
	nactions = LIST_SIZE(actions);
}

static void
init_atoms(void)
{
	_INIT_ATOM(karuiwm.dpy, atoms,    WM_PROTOCOLS);
	_INIT_ATOM(karuiwm.dpy, atoms,    WM_DELETE_WINDOW);
	_INIT_ATOM(karuiwm.dpy, atoms,    WM_STATE);
	_INIT_ATOM(karuiwm.dpy, atoms,    WM_TAKE_FOCUS);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_ACTIVE_WINDOW);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_SUPPORTED);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_NAME);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STATE);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STATE_FULLSCREEN);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STATE_HIDDEN);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_WINDOW_TYPE);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_WINDOW_TYPE_DIALOG);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STRUT);
	_INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STRUT_PARTIAL);
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

static void
parse_args(int argc, char **argv)
{
	int i;
	char *opt;

	set_log_level(LOG_NORMAL);
	for (i = 1; i < argc; ++i) {
		opt = argv[i];
		if (strcmp(opt, "-v") == 0) {
			set_log_level(LOG_VERBOSE);
			VERBOSE("verbose log level");
		} else if (strcmp(opt, "-d") == 0) {
			set_log_level(LOG_DEBUG);
			DEBUG("debug log level");
		} else if (strcmp(opt, "-q") == 0) {
			set_log_level(LOG_FATAL);
		} else {
			fprintf(stderr, "Usage: %s [-v|-d|-q]",
			        karuiwm.env.APPNAME);
			FATAL("Unknown option: %s\n", argv[i]);
		}
	}
}

static void
run(void)
{
	XEvent xe;

	karuiwm.running = true;
	while (karuiwm.running) {
		if (XNextEvent(karuiwm.dpy, &xe) != 0) {
			FATAL("failed to fetch next X event");
			break;
		}
		//DEBUG("run(): e.type = %d", xe.type);
		if (handle[xe.type] != NULL)
			handle[xe.type](&xe);
	}
}

static void
sigchld(int s)
{
	(void) s;

	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		FATAL("could not install SIGCHLD handler");
	/* pid -1 makes it equivalent to wait() (wait for all children);
	 * the purpose of waitpid() is to add WNOHANG */
	while (waitpid(-1, NULL, WNOHANG) > 0);
}

static void
term(void)
{
	char sid[BUFSIZ];
	struct action *a;

	while (nactions > 0) {
		a = actions;
		LIST_REMOVE(&actions, a);
		--nactions;
		action_delete(a);
	}
	focus_delete(karuiwm.focus);
	if (karuiwm.restarting)
		session_save(karuiwm.session, sid, sizeof(sid));
	session_delete(karuiwm.session);
	cursor_delete(karuiwm.cursor);
	layout_term();
	config_term();

	XUngrabKey(karuiwm.dpy, AnyKey, AnyModifier, karuiwm.root);
	XSetInputFocus(karuiwm.dpy, PointerRoot, RevertToPointerRoot,
	               CurrentTime);
	XCloseDisplay(karuiwm.dpy);
}

int
main(int argc, char **argv)
{
	karuiwm.env.APPNAME = "karuiwm";
	parse_args(argc, argv);
	init();
	run();
	term();
	check_restart(argv);
	return EXIT_SUCCESS;
}
