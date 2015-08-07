#include "karuiwm.h"
#include "util.h"
#include "client.h"
#include "desktop.h"
#include "workspace.h"
#include "session.h"
#include "monitor.h"
#include "focus.h"
#include "cursor.h"
#include "layout.h"
#include "actions.h"
#include "xresources.h"
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
static void handle_keyrelease(XEvent *xe);
static void handle_mappingnotify(XEvent *xe);
static void handle_maprequest(XEvent *xe);
static void handle_propertynotify(XEvent *xe);
static void handle_unmapnotify(XEvent *xe);
static int handle_xerror(Display *dpy, XErrorEvent *xe);
static void init(void);
static void init_atoms(void);
static void parse_args(int argc, char **argv);
static void run(void);
static void sigchld(int);
static void term(void);

/* variables */
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
	{ MODKEY,             XK_space,   action_steplayout,  { .i=+1 } },
	{ MODKEY|ShiftMask,   XK_space,   action_steplayout,  { .i=-1 } },

	/* desktop */
	{ MODKEY|ControlMask, XK_h,       action_stepdesktop, { .i=LEFT } },
	{ MODKEY|ControlMask, XK_l,       action_stepdesktop, { .i=RIGHT } },
	{ MODKEY|ControlMask, XK_k,       action_stepdesktop, { .i=UP } },
	{ MODKEY|ControlMask, XK_j,       action_stepdesktop, { .i=DOWN } },

	/* session */
	{ MODKEY,             XK_q,       action_restart,     { 0 } },
	{ MODKEY|ShiftMask,   XK_q,       action_stop,        { 0 } },
};
static struct button buttons[] = {
	{ MODKEY,             Button1,    action_mousemove,   { 0 } },
	{ MODKEY,             Button3,    action_mouseresize, { 0 } },
};

/* implementation */
static void
check_restart(char **argv)
{
	if (karuiwm.state == RESTARTING) {
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

	XUngrabKey(karuiwm.dpy, AnyKey, AnyModifier, karuiwm.root);
	for (i = 0; i < LENGTH(keys); i++)
		XGrabKey(karuiwm.dpy, XKeysymToKeycode(karuiwm.dpy,keys[i].key),
		         keys[i].mod, karuiwm.root, True, GrabModeAsync,
		         GrabModeAsync);
}

static void
handle_buttonpress(XEvent *xe)
{
	int unsigned i;
	XButtonEvent *e = &xe->xbutton;

	//EVENT("buttonpress(%lu)", e->window);

	/* TODO define actions for clicks on root window */
	if (e->window == karuiwm.root)
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

	//EVENT("destroynotify(%lu)", e->window);

	if (session_locate_window(karuiwm.session, &c, e->window) < 0)
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

	if (session_locate_window(karuiwm.session, &c, e->window) < 0) {
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
	struct client *selcli = karuiwm.focus->selmon->seldt->selcli;

	//EVENT("focusin(%lu)", e->window);

	if (e->window == karuiwm.root)
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
	client_grab_buttons(c, LENGTH(buttons), buttons);
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

	karuiwm.state = STARTING;

	/* environment */
	karuiwm.env.HOME = getenv("HOME");
	gethostname(karuiwm.env.HOSTNAME, BUFSIZE_HOSTNAME);
	if (xresources_init(APPNAME) < 0)
		FATAL("could not initialise X resources");

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

	/* input (mouse, keyboard) */
	karuiwm.cursor = cursor_new();
	grabkeys();

	/* layouts, session, focus */
	layout_init();
	karuiwm.session = session_new();
	karuiwm.focus = focus_new(karuiwm.session);
}

static void
init_atoms(void)
{
	INIT_ATOM(karuiwm.dpy, atoms,    WM_PROTOCOLS);
	INIT_ATOM(karuiwm.dpy, atoms,    WM_DELETE_WINDOW);
	INIT_ATOM(karuiwm.dpy, atoms,    WM_STATE);
	INIT_ATOM(karuiwm.dpy, atoms,    WM_TAKE_FOCUS);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_ACTIVE_WINDOW);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_SUPPORTED);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_NAME);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STATE);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STATE_FULLSCREEN);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STATE_HIDDEN);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_WINDOW_TYPE);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_WINDOW_TYPE_DIALOG);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STRUT);
	INIT_ATOM(karuiwm.dpy, netatoms, _NET_WM_STRUT_PARTIAL);
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
			puts("Usage: "APPNAME" [-v|-d|-q]");
			FATAL("Unknown option: %s\n", argv[i]);
		}
	}
}

static void
run(void)
{
	XEvent xe;

	karuiwm.state = RUNNING;
	while (karuiwm.state == RUNNING) {
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

	focus_delete(karuiwm.focus);
	if (karuiwm.state == RESTARTING)
		session_save(karuiwm.session, sid, sizeof(sid));
	session_delete(karuiwm.session);
	cursor_delete(karuiwm.cursor);
	layout_term();
	xresources_term();

	XUngrabKey(karuiwm.dpy, AnyKey, AnyModifier, karuiwm.root);
	XSetInputFocus(karuiwm.dpy, PointerRoot, RevertToPointerRoot,
	               CurrentTime);
	XCloseDisplay(karuiwm.dpy);
}

int
main(int argc, char **argv)
{
	parse_args(argc, argv);
	init();
	run();
	term();
	check_restart(argv);
	return EXIT_SUCCESS;
}
