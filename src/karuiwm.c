#include "karuiwm.h"
#include "util.h"
#include "client.h"
#include "desktop.h"

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
#include <X11/cursorfont.h>

/* macros */
#define INTERSECT(MON, X, Y, W, H) \
        ((MAX(0, MIN((X) + (W), (MON)->x + (MON)->w) - MAX((MON)->x, X))) * \
         (MAX(0, MIN((Y) + (H), (MON)->y + (MON)->h) - MAX((MON)->y, Y))))
#define MOUSEMASK (BUTTONMASK|PointerMotionMask)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLAST };
enum { Left, Right, Up, Down, NoDirection };

/* functions */
static void action_killclient(union argument *arg);
static void action_movemouse(union argument *);
static void action_quit(union argument *arg);
static void action_restart(union argument *arg);
static void action_setmfact(union argument *arg);
static void action_setnmaster(union argument *arg);
static void action_shiftclient(union argument *arg);
static void action_spawn(union argument *arg);
static void action_stepfocus(union argument *arg);
static void action_togglefloat(union argument *arg);
static void action_zoom(union argument *arg);
static void grabkeys(void);
static void handle_buttonpress(XEvent *xe);
static void handle_clientmessage(XEvent *xe);
static void handle_configurerequest(XEvent *xe);
static void handle_configurenotify(XEvent *xe);
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
static void run(void);
static void setupsession(void);
static void sigchld(int);
static void term(void);

/* variables */
static struct desktop *seldt;
static bool running, restarting;              /* application state */
static int screen;                            /* screen */
static Cursor cursor[CurLAST];                /* cursors */
static char const *termcmd[] = { "urxvt", NULL };
static char const *scrotcmd[] = { "prtscr", NULL };

/* event handlers, as array to allow O(1) access; codes in X.h */
static void (*handle[LASTEvent])(XEvent *) = {
	[ButtonPress]      = handle_buttonpress,      /* 4*/
	[ClientMessage]    = handle_clientmessage,    /*33*/
	[ConfigureNotify]  = handle_configurenotify,  /*22*/
	[ConfigureRequest] = handle_configurerequest, /*23*/
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
	{ MODKEY,           XK_n,       action_spawn,       { .v=termcmd } },
	{ MODKEY,           XK_Print,   action_spawn,       { .v=scrotcmd } },

	/* windows */
	{ MODKEY,           XK_j,       action_stepfocus,   { .i=+1 } },
	{ MODKEY,           XK_k,       action_stepfocus,   { .i=-1 } },
	{ MODKEY,           XK_l,       action_setmfact,    { .f=+0.02f } },
	{ MODKEY,           XK_h,       action_setmfact,    { .f=-0.02f } },
	{ MODKEY,           XK_t,       action_togglefloat, { 0 } },
	{ MODKEY|ShiftMask, XK_c,       action_killclient,  { 0 } },

	/* layout */
	{ MODKEY|ShiftMask, XK_j,       action_shiftclient, { .i=+1 } },
	{ MODKEY|ShiftMask, XK_k,       action_shiftclient, { .i=-1 } },
	{ MODKEY,           XK_comma,   action_setnmaster,  { .i=+1 } },
	{ MODKEY,           XK_period,  action_setnmaster,  { .i=-1 } },
	{ MODKEY,           XK_Return,  action_zoom,        { 0 } },

	/* session */
	{ MODKEY,           XK_q,       action_restart,     { 0 } },
	{ MODKEY|ShiftMask, XK_q,       action_quit,        { 0 } },
};
static struct button buttons[] = {
	{ MODKEY,           Button1,    action_movemouse,   { 0 } },
};

/* Handler for killing a client.
 */
static void
action_killclient(union argument *arg)
{
	(void) arg;

	if (seldt->selcli != NULL)
		client_kill(seldt->selcli);
}

/* Handler for moving a client with the mouse.
 */
/* TODO split up */
static void
action_movemouse(union argument *arg)
{
	XEvent ev;
	Window w;
	struct client *c = seldt->selcli;
	int cx = c->x, cy = c->y, x, y, i;
	int unsigned ui;
	long evmask = MOUSEMASK | ExposureMask | SubstructureRedirectMask;
	(void) arg;

	//EVENT("movemouse(%lu)", c->win);

	if (c->state == STATE_FULLSCREEN)
		return;
	desktop_float_client(seldt, c, true);

	/* grab the pointer and change the cursor appearance */
	i = XGrabPointer(kwm.dpy, kwm.root, true, MOUSEMASK, GrabModeAsync,
		         GrabModeAsync, None, cursor[CurMove], CurrentTime);
	if (i != GrabSuccess) {
		WARN("XGrabPointer() failed");
		return;
	}

	/* get initial pointer position */
	if (!XQueryPointer(kwm.dpy, kwm.root, &w, &w, &x, &y, &i, &i, &ui)) {
		WARN("XQueryPointer() failed");
		goto action_movemouse_out;
	}

	/* handle motions */
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
			cx = cx + (ev.xmotion.x - x);
			cy = cy + (ev.xmotion.y - y);
			x = ev.xmotion.x;
			y = ev.xmotion.y;
			client_move(c, cx, cy);
			break;
		default:
			WARN("unhandled event %d", ev.type);
		}
	} while (ev.type != ButtonRelease);
 action_movemouse_out:
	XUngrabPointer(kwm.dpy, CurrentTime);
}

/* Handler for quitting karuiwm.
 */
static void
action_quit(union argument *arg)
{
	(void) arg;

	running = false;
}

/* Handler for restarting karuiwm.
 */
static void
action_restart(union argument *arg)
{
	restarting = true;
	action_quit(arg);
}

/* Handler for changing the factor the master area takes in the layout.
 */
static void
action_setmfact(union argument *arg)
{
	desktop_set_mfact(seldt, seldt->mfact + arg->f);
	desktop_arrange(seldt);
}

/* Handler for changing the number of clients in the master area.
 */
static void
action_setnmaster(union argument *arg)
{
	if (arg->i < 0 && (size_t) (-arg->i) > seldt->nmaster)
		return;
	desktop_set_nmaster(seldt,
	                    (size_t) ((int signed) seldt->nmaster + arg->i));
	desktop_arrange(seldt);
}

/* Handler for moving around clients in the layout/client order/list.
 */
/* TODO move to desktop_swap_clients */
static void
action_shiftclient(union argument *arg)
{
	int unsigned pos, npos;

	if (seldt->selcli == NULL)
		return;
	if (!desktop_locate_client(seldt, seldt->selcli, &pos)) {
		WARN("attempt to shift from unhandled client");
		return;
	}
	if (!desktop_locate_neighbour(seldt, seldt->selcli, pos, arg->i,
	                              NULL, &npos))
		return;
	seldt->clients[pos] = seldt->clients[npos];
	seldt->clients[npos] = seldt->selcli;
	desktop_arrange(seldt);
}

/* Handler for launching a command with the `exec' system call.
 */
static void
action_spawn(union argument *arg)
{
	char *const *cmd = (char *const *) arg->v;

	pid_t pid = fork();
	if (pid == 0) {
		execvp(cmd[0], cmd);
		DIE("execvp(%s) failed: %s", cmd[0], strerror(errno));
	}
	if (pid < 0)
		WARN("fork() failed with code %d", pid);
}

/* Handler for changing the X input focus.
 */
static void
action_stepfocus(union argument *arg)
{
	int unsigned pos, npos;

	if (seldt->selcli == NULL)
		return;
	if (!desktop_locate_client(seldt, seldt->selcli, &pos)) {
		WARN("attempt to step from unfocused client");
		return;
	}
	npos = (pos + (int unsigned) ((int signed) seldt->nc + arg->i))
               % (int unsigned) seldt->nc;
	seldt->selcli = seldt->clients[npos];
	desktop_update_focus(seldt);
}

/* Handler for toggling the selected client's floating mode.
 */
static void
action_togglefloat(union argument *arg)
{
	(void) arg;

	if (seldt->selcli == NULL)
		return;
	desktop_float_client(seldt, seldt->selcli, !seldt->selcli->floating);
}

/* Handler for `zooming' the selected client (moving it to the master area).
 */
static void
action_zoom(union argument *arg)
{
	int unsigned pos;
	(void) arg;

	if (seldt->nc < 2)
		return;

	if (!desktop_locate_client(seldt, seldt->selcli, &pos)) {
		WARN("attempt to zoom unhandled client");
		return;
	}
	if (seldt->selcli->floating)
		return;
	if (pos == seldt->imaster) {
		/* window is at the top: swap with next below */
		seldt->clients[seldt->imaster] = seldt->clients[seldt->imaster + 1];
		seldt->clients[seldt->imaster + 1] = seldt->selcli;
		seldt->selcli = seldt->clients[seldt->imaster];
		desktop_update_focus(seldt);
	} else {
		/* window is somewhere else: swap with top */
		seldt->clients[pos] = seldt->clients[seldt->imaster];
		seldt->clients[seldt->imaster] = seldt->selcli;
	}
	desktop_arrange(seldt);
}

/* Grab all key combinations as defined in the `keys' array to trigger a
 * keypress event.
 */
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

/* X event handler for special mouse button presses.
 */
static void
handle_buttonpress(XEvent *xe)
{
	int unsigned i;
	struct client *c;
	XButtonEvent *e = &xe->xbutton;

	//EVENT("buttonpress(%lu)", e->window);

	if (!desktop_locate_window(seldt, e->window, &c, NULL)) {
		WARN("click on unhandled window");
		return;
	}
	seldt->selcli = c;
	desktop_update_focus(seldt);
	for (i = 0; i < LENGTH(buttons); ++i)
		if (buttons[i].mod == e->state
		&& buttons[i].button == e->button
		&& buttons[i].func != NULL)
			buttons[i].func(&buttons[i].arg);
}

/* X event handler for client messages.
 */
static void
handle_clientmessage(XEvent *xe)
{
	bool fullscreen;
	struct client *c;
	XClientMessageEvent *e = &xe->xclient;

	//EVENT("clientmessage(%lu)", e->window);

	if (!desktop_locate_window(seldt, e->window, &c, NULL))
		return;
	if (e->message_type != netatom[NetWMState]) {
		WARN("received client message for other than WM state");
		return;
	}
	if ((Atom) e->data.l[1] == netatom[NetWMStateFullscreen]
	|| (Atom) e->data.l[2] == netatom[NetWMStateFullscreen]) {
		/* _NET_WM_STATE_ADD || _NET_WM_STATE_TOGGLE */
		fullscreen = e->data.l[0] == 1 ||
			    (e->data.l[0] == 2 &&
			     c->state != STATE_FULLSCREEN);
		desktop_fullscreen_client(seldt, c, fullscreen);
	}
}

/* X event handler for client configuration changes. We only react on root
 * window changes.
 */
static void
handle_configurenotify(XEvent *xe)
{
	XConfigureEvent *e = &xe->xconfigure;

	//EVENT("configurenotify(%lu)", e->window);

	if (e->window == kwm.root)
		/* TODO separate function */
		desktop_update_geometry(seldt, 0, 0,
		                 (int unsigned) DisplayWidth(kwm.dpy, screen),
		                 (int unsigned) DisplayHeight(kwm.dpy, screen));
}

/* X event handler for client configuartion change requests. We simply adapt the
 * client size and forward the otherwise unmodified request to the X server.
 */
static void
handle_configurerequest(XEvent *xe)
{
	XWindowChanges wc;
	XConfigureRequestEvent *e = &xe->xconfigurerequest;

	//EVENT("configurerequest(%lu)", e->window);

	wc.x = e->x;
	wc.y = e->y;
	wc.width = e->width;
	wc.height = e->height;
	wc.border_width = e->border_width;
	wc.sibling = e->above;
	wc.stack_mode = e->detail;
	XConfigureWindow(kwm.dpy, e->window,
			 (int unsigned) e->value_mask, &wc);
}

/* X event handler for the mouse entering a client's area.
 */
static void
handle_enternotify(XEvent *xe)
{
	struct client *c;
	XEnterWindowEvent *e = &xe->xcrossing;

	//EVENT("enternotify(%lu)", e->window);

	if (!desktop_locate_window(seldt, e->window, &c, NULL)) {
		WARN("attempt to enter unhandled/invisible window %lu",
		     e->window);
		return;
	}
	seldt->selcli = c;
	desktop_update_focus(seldt);
}

/* X event handler for windows whose surfaces got exposed (= newly visible) and
 * need a redraw.
 */
static void
handle_expose(XEvent *xe)
{
	XExposeEvent *e = &xe->xexpose;

	//EVENT("expose(%lu)", e->window);
	(void) e;

	/* TODO */
}

/* X event handler for windows that gain the focus without any prior action
 * (this is a fix for VTE terminals that tend to steal the focus).
 */
static void
handle_focusin(XEvent *xe)
{
	XFocusInEvent *e = &xe->xfocus;

	//EVENT("focusin(%lu)", e->window);

	if (e->window == kwm.root)
		return;
	if (seldt->nc == 0 || e->window != seldt->selcli->win) {
		WARN("attempt to steal focus by window %lu (focus is on %lu)",
		      e->window, seldt->nc > 0 ? seldt->selcli->win : 0);
		desktop_update_focus(seldt);
	}
}

/* X event handler for special keys (keys defined in the `keys' array) that have
 * been pressed.
 */
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

/* X event handler for special keys that have been released.
 */
static void
handle_keyrelease(XEvent *xe)
{
	XKeyReleasedEvent *e = &xe->xkey;
	(void) e;

	//EVENT("keyrelease()");

	/* TODO keyrelease might not be needed, remove handler? */
}

/* X event handler for reacting on keyboard mapping changes on a window.
 */
static void
handle_mappingnotify(XEvent *xe)
{
	XMappingEvent *e = &xe->xmapping;

	//EVENT("mappingnotify(%lu)", e->window);
	(void) e;

	/* TODO handle keyboard mapping changes */
}

/* X event handler for windows that wish to be mapped.
 */
static void
handle_maprequest(XEvent *xe)
{
	struct client *c = NULL;
	XMapRequestEvent *e = &xe->xmaprequest;

	//EVENT("maprequest(%lu)", e->window);

	/* window is remapped */
	if (desktop_locate_window(seldt, e->window, &c, NULL)) {
		desktop_detach_client(seldt, c);
		client_delete(c);
	}

	/* initialise */
	c = client_new(e->window);
	if (c == NULL)
		return;

	/* fix floating dimensions */
	if (c->floating)
		client_moveresize(c, MAX(c->x, 0), MAX(c->y, 0),
		                     MIN(c->w, seldt->w), MIN(c->h, seldt->h));

	XMapWindow(kwm.dpy, c->win);
	client_grab_buttons(c, LENGTH(buttons), buttons);
	desktop_attach_client(seldt, c);
	desktop_arrange(seldt);
	desktop_update_focus(seldt);
}

/* X event handler for client property changes.
 */
static void
handle_propertynotify(XEvent *xe)
{
	XPropertyEvent *e = &xe->xproperty;
	struct client *c;

	//EVENT("propertynotify(%lu)", e->window);

	if (!desktop_locate_window(seldt, e->window, &c, NULL))
		return;

	if (e->state == PropertyDelete) {
		/* dwm ignores this */
		NOTICE("property_delete");
		return;
	}

	switch (e->atom) {
	case XA_WM_TRANSIENT_FOR:
		DEBUG("transient property changed for window %lu", c->win);
		client_query_transient(c);
		desktop_restack(seldt);
		desktop_arrange(seldt);
		break;
	case XA_WM_NORMAL_HINTS:
		//DEBUG("size hints changed for window %lu", c->win);
		client_query_sizehints(c);
		break;
	case XA_WM_HINTS:
		WARN("urgent hint changed for window %lu", c->win);
		/* TODO implement urgent hint handling */
		break;
	}
	if (e->atom == XA_WM_NAME || e->atom == netatom[NetWMName])
		client_query_name(c);
	if (e->atom == netatom[NetWMWindowType]) {
		client_query_fullscreen(c);
		client_query_dialog(c);
		desktop_restack(seldt);
		desktop_arrange(seldt);
	}
}

/* X event handler for windows that wish to be unmapped.
 */
static void
handle_unmapnotify(XEvent *xe)
{
	struct client *c;
	XUnmapEvent *e = &xe->xunmap;

	//EVENT("unmapnotify(%lu)", e->window);

	if (!desktop_locate_window(seldt, e->window, &c, NULL))
		return;

	desktop_detach_client(seldt, c);
	client_delete(c);
	desktop_arrange(seldt);
	desktop_update_focus(seldt);
}

/* X error handler. We catch BadWindow errors (as they are often caused by
 * clients) and pass the others to the default X error handler.
 */
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

/* Initialise karuiwm.
 */
static void
init(void)
{
	XSetWindowAttributes wa;

	/* connect to X */
	kwm.dpy = XOpenDisplay(NULL);
	if (kwm.dpy == NULL)
		DIE("could not open X");

	/* errors, zombies, locale */
	xerrorxlib = XSetErrorHandler(handle_xerror);
	sigchld(0);
	if (setlocale(LC_ALL, "") == NULL)
		DIE("could not set locale");
	if (!XSupportsLocale())
		DIE("X does not support locale");

	/* kwm.root window, graphic context, atoms */
	screen = DefaultScreen(kwm.dpy);
	kwm.root = RootWindow(kwm.dpy, screen);
	init_atoms();

	/* events */
	wa.event_mask = SubstructureNotifyMask | SubstructureRedirectMask |
	                PropertyChangeMask | FocusChangeMask | ButtonPressMask |
	                KeyPressMask | PointerMotionMask | StructureNotifyMask;
	XChangeWindowAttributes(kwm.dpy, kwm.root, CWEventMask, &wa);

	/* input (mouse, keyboard) */
	cursor[CurNormal] = XCreateFontCursor(kwm.dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(kwm.dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(kwm.dpy, XC_fleur);
	wa.cursor = cursor[CurNormal];
	XChangeWindowAttributes(kwm.dpy, kwm.root, CWCursor, &wa);
	grabkeys();

	/* session */
	setupsession();
}

/* Initialise the Atoms list.
 */
static void
init_atoms(void)
{
	wmatom[WMProtocols] = XInternAtom(kwm.dpy, "WM_PROTOCOLS", false);
	wmatom[WMDeleteWindow] = XInternAtom(kwm.dpy, "WM_DELETE_WINDOW", false);
	wmatom[WMState] = XInternAtom(kwm.dpy, "WM_STATE", false);
	wmatom[WMTakeFocus] = XInternAtom(kwm.dpy, "WM_TAKE_FOCUS", false);
	netatom[NetActiveWindow] = XInternAtom(kwm.dpy, "_NET_ACTIVE_WINDOW", false);
	netatom[NetSupported] = XInternAtom(kwm.dpy, "_NET_SUPPORTED", false);
	netatom[NetWMName] = XInternAtom(kwm.dpy, "_NET_WM_NAME", false);
	netatom[NetWMState] = XInternAtom(kwm.dpy, "_NET_WM_STATE", false);
	netatom[NetWMStateFullscreen] = XInternAtom(kwm.dpy, "_NET_WM_STATE_FULLSCREEN", false);
	netatom[NetWMWindowType] = XInternAtom(kwm.dpy, "_NET_WM_WINDOW_TYPE", false);
	netatom[NetWMWindowTypeDialog] = XInternAtom(kwm.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", false);
}

/* Event loop.
 */
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

/* Initialise the WM session (scan for pre-existing windows on the X server).
 */
static void
setupsession(void)
{
	int unsigned i, nwins;
	Window p, r, *wins = NULL;
	struct client *c;

	/* TODO: move to workspace */
	seldt = desktop_new();

	/* scan existing windows */
	if (!XQueryTree(kwm.dpy, kwm.root, &r, &p, &wins, &nwins)) {
		WARN("XQueryTree() failed");
		return;
	}

	/* create clients if they do not already exist */
	for (i = 0; i < nwins; i++) {
		c = client_new(wins[i]);
		if (c != NULL) {
			NOTICE("detected existing window %lu", c->win);
			desktop_attach_client(seldt, c);
		}
	}
	desktop_update_focus(seldt);

	/* setup monitors (TODO separate function) */
	desktop_update_geometry(seldt, 0, 0,
	                        (int unsigned) DisplayWidth(kwm.dpy, screen),
	                        (int unsigned) DisplayHeight(kwm.dpy, screen));
}

/* Signal handler. Handle children processes that have exited in order to avoid
 * zombie processes.
 */
static void
sigchld(int s)
{
	(void) s;

	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		DIE("could not install SIGCHLD handler");
	/* pid -1 makes it equivalent to wait() (wait for all children);
	 * here we just add WNOHANG */
	while (0 < waitpid(-1, NULL, WNOHANG));
}

/* Properly delete karuiwm and its elements.
 */
static void
term(void)
{
	desktop_delete(seldt);
	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, kwm.root);
	XFreeCursor(kwm.dpy, cursor[CurNormal]);
	XFreeCursor(kwm.dpy, cursor[CurResize]);
	XFreeCursor(kwm.dpy, cursor[CurMove]);
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
