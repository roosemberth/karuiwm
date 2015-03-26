#include "karuiwm.h"
#include "util.h"
#include "client.h"
#include "desktop.h"
#include "workspace.h"
#include "session.h"
#include "monitor.h"
#include "focus.h"
#include "cursor.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
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
#define INIT_ATOM(D, L, A) L[A] = XInternAtom(D, #A, false)

/* functions */
static void action_killclient(union argument *arg);
static void action_mousemove(union argument *arg, Window win);
static void action_mouseresize(union argument *arg, Window win);
static void action_quit(union argument *arg);
static void action_restart(union argument *arg);
static void action_setmfact(union argument *arg);
static void action_setnmaster(union argument *arg);
static void action_shiftclient(union argument *arg);
static void action_spawn(union argument *arg);
static void action_stepclient(union argument *arg);
static void action_stepdesktop(union argument *arg);
static void action_togglefloat(union argument *arg);
static void action_zoom(union argument *arg);
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
static void handle_keyrelease(XEvent *xe);
static void handle_mappingnotify(XEvent *xe);
static void handle_maprequest(XEvent *xe);
static void handle_propertynotify(XEvent *xe);
static void handle_unmapnotify(XEvent *xe);
static int handle_xerror(Display *dpy, XErrorEvent *xe);
static void init(void);
static void init_atoms(void);
static void mouse_move(struct client *c, int mx, int my,
                       int cx, int cy, int unsigned cw, int unsigned ch);
static void mouse_moveresize(struct client *c, void (*mh)(struct client *,
                             int, int, int, int, int unsigned, int unsigned));
static void mouse_resize(struct client *c, int mx, int my,
                         int cx, int cy, int unsigned cw, int unsigned ch);
static void run(void);
static void sigchld(int);
static void term(void);

/* variables */
static struct session *session;
static struct focus *focus;
static struct cursor *cursor;
static bool running, restarting;    /* application state */
static char const *termcmd[] = { "urxvt", NULL };
static char const *scrotcmd[] = { "prtscr", NULL };

/* event handlers, as array to allow O(1) access; codes in X.h */
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
	[KeyRelease]       = handle_keyrelease,       /* 3*/
	[MapRequest]       = handle_maprequest,       /*20*/
	[MappingNotify]    = handle_mappingnotify,    /*34*/
	[PropertyNotify]   = handle_propertynotify,   /*28*/
	[UnmapNotify]      = handle_unmapnotify       /*18*/
};
static int (*xerrorxlib)(Display *dpy, XErrorEvent *xe);

static struct key keys[] = {
	/* applications */
	{ MODKEY,             XK_n,       action_spawn,       { .v=termcmd } },
	{ MODKEY,             XK_Print,   action_spawn,       { .v=scrotcmd } },

	/* windows */
	{ MODKEY,             XK_j,       action_stepclient,   { .i=+1 } },
	{ MODKEY,             XK_k,       action_stepclient,   { .i=-1 } },
	{ MODKEY,             XK_l,       action_setmfact,    { .f=+0.02f } },
	{ MODKEY,             XK_h,       action_setmfact,    { .f=-0.02f } },
	{ MODKEY,             XK_t,       action_togglefloat, { 0 } },
	{ MODKEY|ShiftMask,   XK_c,       action_killclient,  { 0 } },

	/* layout */
	{ MODKEY|ShiftMask,   XK_j,       action_shiftclient, { .i=+1 } },
	{ MODKEY|ShiftMask,   XK_k,       action_shiftclient, { .i=-1 } },
	{ MODKEY,             XK_comma,   action_setnmaster,  { .i=+1 } },
	{ MODKEY,             XK_period,  action_setnmaster,  { .i=-1 } },
	{ MODKEY,             XK_Return,  action_zoom,        { 0 } },

	/* desktop */
	{ MODKEY|ControlMask, XK_h,       action_stepdesktop, { .i=LEFT } },
	{ MODKEY|ControlMask, XK_l,       action_stepdesktop, { .i=RIGHT } },
	{ MODKEY|ControlMask, XK_k,       action_stepdesktop, { .i=UP } },
	{ MODKEY|ControlMask, XK_j,       action_stepdesktop, { .i=DOWN } },

	/* session */
	{ MODKEY,             XK_q,       action_restart,     { 0 } },
	{ MODKEY|ShiftMask,   XK_q,       action_quit,        { 0 } },
};
static struct button buttons[] = {
	{ MODKEY,             Button1,    action_mousemove,   { 0 } },
	{ MODKEY,             Button3,    action_mouseresize, { 0 } },
};

/* implementation */
static void
action_killclient(union argument *arg)
{
	(void) arg;

	desktop_kill_client(focus->selmon->seldt);
}

static void
action_mousemove(union argument *arg, Window win)
{
	struct client *c;
	(void) arg;

	//EVENT("movemouse(%lu)", c->win);

	if (session_locate_window(session, &c, win) < 0) {
		WARN("attempt to mouse-move unhandled window %lu", win);
		return;
	}
	mouse_moveresize(focus->selmon->seldt->selcli, mouse_move);
}

static void
action_mouseresize(union argument *arg, Window win)
{
	struct client *c;
	(void) arg;

	//EVENT("resizemouse(%lu)", c->win);

	if (session_locate_window(session, &c, win) < 0) {
		WARN("attempt to mouse-resize unhandled window %lu", win);
		return;
	}
	mouse_moveresize(c, mouse_resize);
}

static void
action_quit(union argument *arg)
{
	(void) arg;

	running = false;
}

static void
action_restart(union argument *arg)
{
	restarting = true;
	action_quit(arg);
}

static void
action_setmfact(union argument *arg)
{
	struct desktop *d = focus->selmon->seldt;

	desktop_set_mfact(d, d->mfact + arg->f);
	desktop_arrange(d);
}

static void
action_setnmaster(union argument *arg)
{
	struct desktop *d = focus->selmon->seldt;

	if (arg->i < 0 && (size_t) (-arg->i) > d->nmaster)
		return;
	desktop_set_nmaster(d, (size_t) ((int signed) d->nmaster + arg->i));
	desktop_arrange(d);
}

static void
action_shiftclient(union argument *arg)
{
	struct desktop *d = focus->selmon->seldt;

	if (d->selcli == NULL)
		return;
	(void) desktop_shift_client(d, arg->i);
	desktop_arrange(d);
}

static void
action_spawn(union argument *arg)
{
	char *const *cmd = (char *const *) arg->v;

	pid_t pid = fork();
	if (pid == 0) {
		execvp(cmd[0], cmd);
		FATAL("execvp(%s) failed: %s", cmd[0], strerror(errno));
	}
	if (pid < 0)
		WARN("fork() failed with code %d", pid);
}

static void
action_stepclient(union argument *arg)
{
	desktop_step_client(focus->selmon->seldt, arg->i);
}

static void
action_stepdesktop(union argument *arg)
{
	monitor_step_desktop(focus->selmon, arg->i);
}

static void
action_togglefloat(union argument *arg)
{
	struct desktop *d = focus->selmon->seldt;
	(void) arg;

	desktop_float_client(d, d->selcli, !d->selcli->floating);
	desktop_arrange(d);
}

static void
action_zoom(union argument *arg)
{
	struct desktop *d = focus->selmon->seldt;
	(void) arg;

	desktop_zoom(d);
	desktop_arrange(d);
}

static void
grabkeys(void)
{
	int unsigned i;

	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, kwm.root);
	for (i = 0; i < LENGTH(keys); i++)
		XGrabKey(kwm.dpy, XKeysymToKeycode(kwm.dpy, keys[i].key),
		         keys[i].mod, kwm.root, True, GrabModeAsync,
		         GrabModeAsync);
}

static void
handle_buttonpress(XEvent *xe)
{
	int unsigned i;
	XButtonEvent *e = &xe->xbutton;

	//EVENT("buttonpress(%lu)", e->window);

	/* TODO define actions for clicks on root window */
	if (e->window == kwm.root)
		return;

	for (i = 0; i < LENGTH(buttons); ++i) {
		if (buttons[i].mod == e->state
		&& buttons[i].button == e->button
		&& buttons[i].func != NULL) {
			buttons[i].func(&buttons[i].arg, e->window);
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

	if (session_locate_window(session, &c, e->window) < 0)
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

	if (e->window == kwm.root)
		focus_scan_monitors(focus);
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
	XConfigureWindow(kwm.dpy, e->window, (int unsigned) e->value_mask, &wc);
}

static void
handle_destroynotify(XEvent *xe)
{
	struct client *c;
	struct desktop *d;
	XDestroyWindowEvent *e = &xe->xdestroywindow;

	//EVENT("destroynotify(%lu)", e->window);

	if (session_locate_window(session, &c, e->window) < 0)
		return;
	d = c->desktop;
	desktop_detach_client(d, c);
	client_delete(c);
	desktop_arrange(d);
	if (d->monitor != NULL)
		desktop_update_focus(d);
}

static void
handle_enternotify(XEvent *xe)
{
	struct client *c;
	XEnterWindowEvent *e = &xe->xcrossing;

	//EVENT("enternotify(%lu)", e->window);

	if (session_locate_window(session, &c, e->window) < 0) {
		WARN("entering unhandled window %lu", e->window);
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
	struct client *selcli = focus->selmon->seldt->selcli;

	//EVENT("focusin(%lu)", e->window);

	if (e->window == kwm.root)
		return;
	if (selcli == NULL || e->window != selcli->win) {
		WARN("attempt to steal focus by window %lu (focus is on %lu)",
		     e->window, selcli == NULL ? 0 : selcli->win);
		desktop_focus_client(selcli->desktop, selcli);
	}
}

static void
handle_keypress(XEvent *xe)
{
	int unsigned i;
	XKeyPressedEvent *e = &xe->xkey;

	//EVENT("keypress()");

	KeySym keysym = XLookupKeysym(e, 0);
	for (i = 0; i < LENGTH(keys); i++) {
		if (e->state == keys[i].mod
		&& keysym == keys[i].key
		&& keys[i].func) {
			keys[i].func(&keys[i].arg);
			break;
		}
	}
}

static void
handle_keyrelease(XEvent *xe)
{
	XKeyReleasedEvent *e = &xe->xkey;

	//EVENT("keyrelease()");

	/* TODO keyrelease might not be needed, remove this handler? */
	(void) e;
}

static void
handle_mappingnotify(XEvent *xe)
{
	XMappingEvent *e = &xe->xmapping;

	//EVENT("mappingnotify(%lu)", e->window);

	/* TODO handle keyboard mapping changes */
	(void) e;
}

static void
handle_maprequest(XEvent *xe)
{
	struct client *c;
	struct desktop *d;
	XMapRequestEvent *e = &xe->xmaprequest;

	//EVENT("maprequest(%lu)", e->window);

	/* window is being remapped */
	if (session_locate_window(session, &c, e->window) == 0) {
		XMapWindow(kwm.dpy, c->win);
		return;
	}

	c = client_new(e->window);
	if (c == NULL)
		return;

	/* fix floating dimensions */
	d = focus->selmon->seldt;
	desktop_attach_client(d, c);
	if (c->floating)
		client_moveresize(c, MAX(c->floatx, 0),
		                     MAX(c->floaty, 0),
		                     MIN(c->floatw, d->monitor->w),
		                     MIN(c->floath, d->monitor->h));
	desktop_arrange(d);
	XMapWindow(kwm.dpy, c->win);
	client_grab_buttons(c, LENGTH(buttons), buttons);
	desktop_focus_client(d, c);
}

static void
handle_propertynotify(XEvent *xe)
{
	XPropertyEvent *e = &xe->xproperty;
	struct client *c;

	//EVENT("propertynotify(%lu)", e->window);

	if (session_locate_window(session, &c, e->window) < 0)
		return;

	if (e->state == PropertyDelete) {
		/* FIXME dwm ignores this */
		NOTICE("property_delete");
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

static void
handle_unmapnotify(XEvent *xe)
{
	XUnmapEvent *e = &xe->xunmap;

	//EVENT("unmapnotify(%lu)", e->window);

	/* TODO necessary to handle unmap events? */
	(void) e;
}

static int
handle_xerror(Display *dpy, XErrorEvent *ee)
{
	char es[256];
	bool ignore;

	XGetErrorText(dpy, ee->error_code, es, 256);
	ERROR("%s after request %d", es, ee->request_code);
	ignore = ee->error_code == BadWindow;

	/* default error handler exits */
	return ignore ? 0 : xerrorxlib(dpy, ee);
}

static void
init(void)
{
	XSetWindowAttributes wa;

	/* connect to X */
	kwm.dpy = XOpenDisplay(NULL);
	if (kwm.dpy == NULL)
		FATAL("could not open X");

	/* errors, zombies, locale */
	xerrorxlib = XSetErrorHandler(handle_xerror);
	sigchld(0);
	if (setlocale(LC_ALL, "") == NULL)
		FATAL("could not set locale");
	if (!XSupportsLocale())
		FATAL("X does not support locale");

	/* X screen, root window, atoms */
	kwm.screen = DefaultScreen(kwm.dpy);
	kwm.root = RootWindow(kwm.dpy, kwm.screen);
	init_atoms();

	/* events */
	wa.event_mask = SubstructureNotifyMask | SubstructureRedirectMask |
	                PropertyChangeMask | FocusChangeMask | ButtonPressMask |
	                KeyPressMask | PointerMotionMask | StructureNotifyMask;
	XChangeWindowAttributes(kwm.dpy, kwm.root, CWEventMask, &wa);

	/* input (mouse, keyboard) */
	cursor = cursor_new();
	grabkeys();

	/* session, focus */
	session = session_new();
	focus = focus_new(session);
}

static void
init_atoms(void)
{
	INIT_ATOM(kwm.dpy, atoms,    WM_PROTOCOLS);
	INIT_ATOM(kwm.dpy, atoms,    WM_DELETE_WINDOW);
	INIT_ATOM(kwm.dpy, atoms,    WM_STATE);
	INIT_ATOM(kwm.dpy, atoms,    WM_TAKE_FOCUS);
	INIT_ATOM(kwm.dpy, netatoms, _NET_ACTIVE_WINDOW);
	INIT_ATOM(kwm.dpy, netatoms, _NET_SUPPORTED);
	INIT_ATOM(kwm.dpy, netatoms, _NET_WM_NAME);
	INIT_ATOM(kwm.dpy, netatoms, _NET_WM_STATE);
	INIT_ATOM(kwm.dpy, netatoms, _NET_WM_STATE_FULLSCREEN);
	INIT_ATOM(kwm.dpy, netatoms, _NET_WM_STATE_HIDDEN);
	INIT_ATOM(kwm.dpy, netatoms, _NET_WM_WINDOW_TYPE);
	INIT_ATOM(kwm.dpy, netatoms, _NET_WM_WINDOW_TYPE_DIALOG);
}

static void
mouse_move(struct client *c, int mx, int my,
          int cx, int cy, int unsigned cw, int unsigned ch)
{
	XEvent ev;
	long evmask = MOUSEMASK | ExposureMask | SubstructureRedirectMask;
	int dx, dy;
	(void) cw;
	(void) ch;

	if (cursor_set_type(cursor, CURSOR_MOVE) < 0)
		WARN("could not change cursor appearance");
	do {
		XMaskEvent(kwm.dpy, evmask, &ev);
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
	if (cursor_set_type(cursor, CURSOR_NORMAL) < 0)
		WARN("could not reset cursor appearance");
	focus_associate_client(focus, c);
}

static void
mouse_moveresize(struct client *c, void (*mh)(struct client *c, int, int,
                 int, int, int unsigned, int unsigned))
{
	int mx, my;

	if (c == NULL || c->state != STATE_NORMAL)
		return;

	if (cursor_get_pos(cursor, &mx, &my) < 0) {
		WARN("could not get cursor position");
		return;
	}
	focus_monitor_by_mouse(focus, mx, my);

	/* fix client position */
	if (c->floatx + (int signed) c->floatw < mx)
		c->floatx = mx - (int signed) c->floatw + 1;
	if (c->floatx > mx)
		c->floatx = mx;
	if (c->floaty + (int signed) c->floath < my)
		c->floaty = my - (int signed) c->floath + 1;
	if (c->floaty > my)
		c->floaty = my;
	desktop_float_client(focus->selmon->seldt, c, true);
	desktop_arrange(focus->selmon->seldt);

	/* handle events */
	mh(c, mx, my, c->floatx, c->floaty, c->floatw, c->floath);
}

static void
mouse_resize(struct client *c, int mx, int my,
            int cx, int cy, int unsigned cw, int unsigned ch)
{
	XEvent ev;
	long evmask = MOUSEMASK | ExposureMask | SubstructureRedirectMask;
	int dx, dy;
	bool left, right, top, bottom;

	/* determine area, set cursor appearance */
	top = my - cy < (int signed) ch / 3;
	bottom = my - cy > 2 * (int signed) ch / 3;
	left = mx - cx < (int signed) cw / 3;
	right = mx - cx > 2 * (int signed) cw / 3;
	if (!top && !bottom && !left && !right)
		return;
	cursor_set_type(cursor, top && left     ? CURSOR_RESIZE_TOP_LEFT     :
	                        top && right    ? CURSOR_RESIZE_TOP_RIGHT    :
	                        bottom && left  ? CURSOR_RESIZE_BOTTOM_LEFT  :
	                        bottom && right ? CURSOR_RESIZE_BOTTOM_RIGHT :
	                        left            ? CURSOR_RESIZE_LEFT         :
	                        right           ? CURSOR_RESIZE_RIGHT        :
	                        top             ? CURSOR_RESIZE_TOP          :
	                        bottom          ? CURSOR_RESIZE_BOTTOM       :
	                        /* ignore */      CURSOR_NORMAL);
	do {
		XMaskEvent(kwm.dpy, evmask, &ev);
		switch (ev.type) {
		case ButtonRelease:
			break;
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handle[ev.type](&ev);
			break;
		case MotionNotify:
			/* FIXME prevent resizing to negative dimensions */
			dx = ev.xmotion.x - mx;
			if (c->incw > 0)
				dx = dx / (int signed) c->incw
				        * (int signed) c->incw;
			dy = ev.xmotion.y - my;
			if (c->inch > 0)
				dy = dy / (int signed) c->inch
				        * (int signed) c->inch;
			if (left) {
				cx += dx;
				cw = (int unsigned) ((int signed) cw - dx);
			} else if (right) {
				cw = (int unsigned) ((int signed) cw + dx);
			}
			if (top) {
				cy += dy;
				ch = (int unsigned) ((int signed) ch - dy);
			} else if (bottom) {
				ch = (int unsigned) ((int signed) ch + dy);
			}
			mx += dx;
			my += dy;
			client_moveresize(c, cx, cy, cw, ch);
			break;
		default:
			WARN("unhandled event %d", ev.type);
		}
	} while (ev.type != ButtonRelease);
	if (cursor_set_type(cursor, CURSOR_NORMAL) < 0)
		WARN("could not reset cursor appearance");
	focus_associate_client(focus, c);
}

static void
run(void)
{
	XEvent xe;

	running = true;
	restarting = false;
	while (running && !XNextEvent(kwm.dpy, &xe)) {
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
	 * here we just add WNOHANG */
	while (0 < waitpid(-1, NULL, WNOHANG));
}

static void
term(void)
{
	char sid[BUFSIZ];

	focus_delete(focus);
	if (restarting)
		session_save(session, sid, sizeof(sid));
	session_delete(session);
	cursor_delete(cursor);

	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, kwm.root);
	XSetInputFocus(kwm.dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XCloseDisplay(kwm.dpy);
}

int
main(int argc, char **argv)
{
	(void) argv;

	set_log_level(LOG_DEBUG);

	if (argc > 1) {
		puts(APPNAME" © 2015 ayekat, see LICENSE for details");
		return EXIT_FAILURE;
	}
	init();
	run();
	term();
	if (restarting) {
		VERBOSE("restarting ...");
		execvp(argv[0], argv);
		ERROR("restart failed: %s", strerror(errno));
	}
	VERBOSE("shutting down ...");
	return EXIT_SUCCESS;
}
