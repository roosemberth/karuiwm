#include "karuiwm.h"
#include "util.h"
#include "client.h"
#include "locate.h"
#include "layout.h"

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
#define APPNAME "karuiwm"
#define CLIENTMASK (EnterWindowMask | PropertyChangeMask | StructureNotifyMask)
#define INTERSECT(MON, X, Y, W, H) \
        ((MAX(0, MIN((X) + (W), (MON)->x + (MON)->w) - MAX((MON)->x, X))) * \
         (MAX(0, MIN((Y) + (H), (MON)->y + (MON)->h) - MAX((MON)->y, Y))))
#define MOUSEMASK (BUTTONMASK|PointerMotionMask)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLAST };
enum { Left, Right, Up, Down, NoDirection };

/* functions */
static void action_killclient(union argument const *arg);
static void action_movemouse(union argument const *);
static void action_quit(union argument const *arg);
static void action_setmfact(union argument const *arg);
static void action_setnmaster(union argument const *arg);
static void action_shiftclient(union argument const *arg);
static void action_spawn(union argument const *arg);
static void action_stepfocus(union argument const *arg);
static void action_togglefloat(union argument const *arg);
static void action_zoom(union argument const *arg);
static void arrange(void);
static void attachclient(struct client *c);
static void detachclient(struct client *c);
static void floatclient(struct client *c, bool floating);
static void fullscreenclient(struct client *c, bool fullscreen);
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
static signed handle_xerror(Display *dpy, XErrorEvent *xe);
static void init(void);
static void init_atoms(void);
static bool locate_neighbour(struct client **n, unsigned *npos,
                             struct client *c, unsigned cpos, signed dir);
static void restack(void);
static void run(void);
static void setupsession(void);
static void sigchld(signed);
static void term(void);
static void updatefocus(void);
static void updategeom(void);

/* variables */
static bool running;                          /* application state */
static signed screen;                            /* screen */
static Window root;                           /* root window */
static Cursor cursor[CurLAST];                /* cursors */
static enum log_level log_level;
static struct client **clients = NULL, *selcli = NULL;
static size_t nc = 0, nmaster = 1;
static unsigned imaster = 0;
static float mfact = 0.5;
static unsigned sw, sh;
static char const *termcmd[] = { "urxvt", NULL };
static char const *scrotcmd[] = { "prtscr", NULL };
static char const *volupcmd[] = { "amixer", "set", "Master", "2+", "unmute", NULL };
static char const *voldowncmd[] = { "amixer", "set", "Master", "2-", "unmute", NULL };
static char const *volmutecmd[] = { "amixer", "set", "Master", "toggle", NULL };

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
static signed (*xerrorxlib)(Display *dpy, XErrorEvent *xe);

static struct key keys[] = {
	/* applications */
	{ MODKEY,           XK_n,       action_spawn,       { .v=termcmd } },
	{ MODKEY,           XK_Print,   action_spawn,       { .v=scrotcmd } },

	/* hardware */
	{ 0,                0x1008FF11, action_spawn,       { .v=voldowncmd } },
	{ 0,                0x1008FF12, action_spawn,       { .v=volmutecmd } },
	{ 0,                0x1008FF13, action_spawn,       { .v=volupcmd } },

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
	{ MODKEY|ShiftMask, XK_q,       action_quit,        { 0 } },
};
static struct button buttons[] = {
	{ MODKEY,           Button1,    action_movemouse,   { 0 } },
};

/* implementation */
static void
action_killclient(union argument const *arg)
{
	(void) arg;

	if (selcli != NULL)
		client_kill(selcli);
}

/* TODO split up */
static void
action_movemouse(union argument const *arg)
{
	XEvent ev;
	Window w;
	struct client *c = selcli;
	signed cx = c->x, cy = c->y, x, y, i;
	unsigned ui;
	long evmask = MOUSEMASK | ExposureMask | SubstructureRedirectMask;
	(void) arg;

	//EVENT("movemouse(%lu)", c->win);

	if (c->state == STATE_FULLSCREEN)
		return;
	if (!c->floating)
		floatclient(c, true);

	/* grab the pointer and change the cursor appearance */
	i = XGrabPointer(kwm.dpy, root, true, MOUSEMASK, GrabModeAsync,
		         GrabModeAsync, None, cursor[CurMove], CurrentTime);
	if (i != GrabSuccess) {
		WARN("XGrabPointer() failed");
		return;
	}

	/* get initial pointer position */
	if (!XQueryPointer(kwm.dpy, root, &w, &w, &x, &y, &i, &i, &ui)) {
		WARN("XQueryPointer() failed");
		return;
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
	XUngrabPointer(kwm.dpy, CurrentTime);
}

static void
action_quit(union argument const *arg)
{
	(void) arg;

	running = false;
}

static void
action_setmfact(union argument const *arg)
{
	mfact = MAX(0.1f, MIN(0.9f, mfact + arg->f));
	arrange();
}

static void
action_setnmaster(union argument const *arg)
{
	if (arg->i < 0 && (size_t) (-arg->i) > nmaster)
		return;
	nmaster = (size_t) ((signed) nmaster + arg->i);
	arrange();
}

static void
action_shiftclient(union argument const *arg)
{
	unsigned pos, npos;

	if (selcli == NULL)
		return;
	if (!locate_client2index(clients, nc, selcli, &pos)) {
		WARN("attempt to shift from unhandled client");
		return;
	}
	if (!locate_neighbour(NULL, &npos, selcli, pos, arg->i))
		return;
	clients[pos] = clients[npos];
	clients[npos] = selcli;
	arrange();
}

static void
action_spawn(union argument const *arg)
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

static void
action_stepfocus(union argument const *arg)
{
	unsigned pos, npos;

	if (selcli == NULL)
		return;
	if (!locate_client2index(clients, nc, selcli, &pos)) {
		WARN("attempt to step from unfocused client");
		return;
	}
	npos = (pos + (unsigned) ((signed) nc + arg->i)) % (unsigned) nc;
	selcli = clients[npos];
	updatefocus();
}

static void
action_togglefloat(union argument const *arg)
{
	(void) arg;

	if (selcli == NULL)
		return;
	floatclient(selcli, !selcli->floating);
}

static void
action_zoom(union argument const *arg)
{
	unsigned pos;
	(void) arg;

	if (nc < 2)
		return;

	if (!locate_client2index(clients, nc, selcli, &pos)) {
		WARN("attempt to zoom unhandled client");
		return;
	}
	if (selcli->floating)
		return;
	if (pos == imaster) {
		/* window is at the top: swap with next below */
		clients[imaster] = clients[imaster + 1];
		clients[imaster + 1] = selcli;
		selcli = clients[imaster];
		updatefocus();
	} else {
		/* window is somewhere else: swap with top */
		clients[pos] = clients[imaster];
		clients[imaster] = selcli;
	}
	arrange();
}

static void
arrange(void)
{
	unsigned i, is = 0;
	size_t ntiled = nc - imaster;
	struct client *c, **tiled = clients + imaster;
	Window stack[nc];

	if (nc == 0)
		return;

	setclientmask(false);
	if (ntiled > 0) {
		/* TODO modular layout management */
		rstack(tiled, ntiled, MIN(nmaster, ntiled), mfact, sw, sh);
	}
	for (i = 0; i < imaster; ++i) {
		c = clients[i];
		client_moveresize(c, c->x, c->y, c->w, c->h);
	}
	for (i = 0; i < nc; ++i) {
		c = clients[i];
		if (c->state == STATE_FULLSCREEN) {
			client_moveresize(c, 0, 0, sw, sh);
			stack[is++] = c->win;
		}
	}
	for (i = 0; i < nc; ++i) {
		c = clients[i];
		if (c->state != STATE_FULLSCREEN)
			stack[is++] = c->win;
	}
	XRestackWindows(kwm.dpy, stack, (signed) nc);
	setclientmask(true);
}

static void
attachclient(struct client *c)
{
	if (c->floating) {
		list_prepend((void ***) &clients, &nc, c, sizeof(c),
		             "client list");
		++imaster;
	} else {
		list_append((void ***) &clients, &nc, c, sizeof(c),
		             "client list");
	}
	selcli = c;
}

static void
detachclient(struct client *c)
{
	signed pos = list_remove((void ***) &clients, &nc, c,
	                         sizeof(struct client), "client list");
	if (pos < 0) {
		WARN("attempt to detach unhandled client(%lu)", c->win);
		return;
	}
	if ((unsigned) pos < imaster)
		--imaster;
	if (c == selcli)
		selcli = nc > 0 ? clients[MIN((unsigned) pos, nc - 1)]
		                : NULL;
}

static void
floatclient(struct client *c, bool floating)
{
	unsigned pos;

	if (c->floating == floating)
		return;

	if (!locate_client2index(clients, nc, c, &pos)) {
		WARN("attempt to float unhandled window");
		return;
	}
	client_set_floating(c, floating);
	list_shift((void **) clients,
	           floating ? 0 : (unsigned) nc - 1, pos);
	imaster = (unsigned) ((signed) imaster + (floating ? 1 : -1));
	arrange();
}

static void
fullscreenclient(struct client *c, bool fullscreen)
{
	if ((c->state == STATE_FULLSCREEN) == fullscreen)
		return;

	client_set_fullscreen(c, fullscreen);
	arrange();
}

static void
grabkeys(void)
{
	unsigned i;

	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, root);
	for (i = 0; i < LENGTH(keys); i++)
		XGrabKey(kwm.dpy, XKeysymToKeycode(kwm.dpy, keys[i].key),
		         keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
}

static void
handle_buttonpress(XEvent *xe)
{
	unsigned i;
	struct client *c;
	XButtonEvent *e = &xe->xbutton;

	EVENT("buttonpress(%lu)", e->window);

	if (!locate_window2client(clients, nc, e->window, &c, NULL)) {
		WARN("click on unhandled window");
		return;
	}

	for (i = 0; i < LENGTH(buttons); i++) {
		if (buttons[i].mod == e->state
		&& buttons[i].button == e->button && buttons[i].func) {
			buttons[i].func(&buttons[i].arg);
		}
	}

	/* update focus */
	selcli = c;
	updatefocus();
}

static void
handle_clientmessage(XEvent *xe)
{
	bool fullscreen, change;
	struct client *c;
	XClientMessageEvent *e = &xe->xclient;

	//EVENT("clientmessage(%lu)", e->window);

	if (!locate_window2client(clients, nc, e->window, &c, NULL))
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
		change = fullscreen != (c->state == STATE_FULLSCREEN);
		if (change)
			fullscreenclient(c, fullscreen);
	}
}

static void
handle_configurenotify(XEvent *xe)
{
	XConfigureEvent *e = &xe->xconfigure;

	//EVENT("configurenotify(%lu)", e->window);

	if (e->window == root)
		updategeom();
}

static void
handle_configurerequest(XEvent *xe)
{
	struct client *c = NULL;
	XWindowChanges wc;
	XConfigureEvent cev;
	XConfigureRequestEvent *e = &xe->xconfigurerequest;

	//EVENT("configurerequest(%lu)", e->window);

	/* forward configuration if unmanaged (or if we don't force the size) */
	(void) locate_window2client(clients, nc, e->window, &c, NULL);
	//if (c == NULL || c->fullscreen || c->floating) {
		wc.x = e->x;
		wc.y = e->y;
		wc.width = e->width;
		wc.height = e->height;
		wc.border_width = e->border_width;
		wc.sibling = e->above;
		wc.stack_mode = e->detail;
		XConfigureWindow(kwm.dpy, e->window,
		                 (unsigned) e->value_mask, &wc);
		return;
	//}

	/* force size with XSendEvent instead of ordinary XConfigureWindow */
	/* TODO necessary? */
	cev.type = ConfigureNotify;
	cev.display = kwm.dpy;
	cev.event = c->win;
	cev.window = c->win;
	cev.x = c->x;
	cev.y = c->y;
	cev.width = (signed) c->w;
	cev.height = (signed) c->h;
	cev.border_width = BORDERWIDTH;
	cev.above = None;
	cev.override_redirect = False;
	XSendEvent(kwm.dpy, c->win, False, StructureNotifyMask, (XEvent *) &cev);
}

static void
handle_enternotify(XEvent *xe)
{
	struct client *c;
	XEnterWindowEvent *e = &xe->xcrossing;

	//EVENT("enternotify(%lu)", e->window);

	if (!locate_window2client(clients, nc, e->window, &c, NULL)) {
		WARN("attempt to enter unhandled/invisible window %u",
		     e->window);
		return;
	}
	selcli = c;
	updatefocus();
}

static void
handle_expose(XEvent *xe)
{
	XExposeEvent *e = &xe->xexpose;

	//EVENT("expose(%lu)", e->window);
	(void) e;

	/* TODO */
}

/* fix for VTE terminals that tend to steal the focus */
static void
handle_focusin(XEvent *xe)
{
	XFocusInEvent *e = &xe->xfocus;

	//EVENT("focusin(%lu)", e->window);

	if (e->window == root)
		return;
	if (nc == 0 || e->window != selcli->win) {
		WARN("focusin on unfocused window %lu (focus is on %lu)",
		      e->window, nc > 0 ? selcli->win : 0);
		updatefocus();
	}
}

static void
handle_keypress(XEvent *xe)
{
	unsigned i;
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
	(void) e;

	//EVENT("keyrelease()");

	/* TODO */
}

static void
handle_mappingnotify(XEvent *xe)
{
	XMappingEvent *e = &xe->xmapping;

	//EVENT("mappingnotify(%lu)", e->window);
	(void) e;

	/* TODO */
}

static void
handle_maprequest(XEvent *xe)
{
	struct client *c = NULL;
	XMapRequestEvent *e = &xe->xmaprequest;

	//EVENT("maprequest(%lu)", e->window);

	/* in case of remap */
	if (locate_window2client(clients, nc, e->window, &c, NULL)) {
		detachclient(c);
		client_delete(c);
	}

	/* initialise */
	c = client_new(e->window);
	if (c == NULL)
		return;

	/* fix floating dimensions */
	if (c->floating)
		client_moveresize(c, MAX(c->x, 0), MAX(c->y, 0),
		                     MIN(c->w, sw), MIN(c->h, sh));

	XMapWindow(kwm.dpy, c->win);
	client_grab_buttons(c, LENGTH(buttons), buttons);
	attachclient(c);
	arrange();
	updatefocus();
}

static void
handle_propertynotify(XEvent *xe)
{
	XPropertyEvent *e = &xe->xproperty;
	struct client *c;

	//EVENT("propertynotify(%lu)", e->window);

	if (!locate_window2client(clients, nc, e->window, &c, NULL))
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
		restack();
		arrange();
		break;
	case XA_WM_NORMAL_HINTS:
		//DEBUG("size hints changed for window %lu", c->win);
		client_query_sizehints(c);
		break;
	case XA_WM_HINTS:
		WARN("urgent hint changed for window %u", c->win);
		/* TODO implement urgent hint handling */
		break;
	}
	if (e->atom == XA_WM_NAME || e->atom == netatom[NetWMName])
		client_query_name(c);
	if (e->atom == netatom[NetWMWindowType]) {
		client_query_fullscreen(c);
		client_query_dialog(c);
		restack();
		arrange();
	}
}

static void
handle_unmapnotify(XEvent *xe)
{
	struct client *c;
	XUnmapEvent *e = &xe->xunmap;

	//EVENT("unmapnotify(%lu)", e->window);

	if (!locate_window2client(clients, nc, e->window, &c, NULL))
		return;

	detachclient(c);
	client_delete(c);
	arrange();
	updatefocus();
}

static signed
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
		DIE("could not open X");

	/* errors, zombies, locale */
	xerrorxlib = XSetErrorHandler(handle_xerror);
	sigchld(0);
	if (setlocale(LC_ALL, "") == NULL)
		DIE("could not set locale");
	if (!XSupportsLocale())
		DIE("X does not support locale");

	/* root window, graphic context, atoms */
	screen = DefaultScreen(kwm.dpy);
	root = RootWindow(kwm.dpy, screen);
	init_atoms();

	/* events */
	wa.event_mask = SubstructureNotifyMask | SubstructureRedirectMask |
	                PropertyChangeMask | FocusChangeMask | ButtonPressMask |
	                KeyPressMask | PointerMotionMask | StructureNotifyMask;
	XChangeWindowAttributes(kwm.dpy, root, CWEventMask, &wa);

	/* input (mouse, keyboard) */
	cursor[CurNormal] = XCreateFontCursor(kwm.dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(kwm.dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(kwm.dpy, XC_fleur);
	wa.cursor = cursor[CurNormal];
	XChangeWindowAttributes(kwm.dpy, root, CWCursor, &wa);
	grabkeys();

	/* session */
	setupsession();
}

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

static bool
locate_neighbour(struct client **n, unsigned *npos,
                 struct client *c, unsigned cpos, signed dir)
{
	unsigned relsrc, reldst, dst;
	size_t offset, mod;
	bool floating = cpos < imaster; /* do not rely on client information */

	if (nc < 2)
		return false;
	if (c->state == STATE_FULLSCREEN)
		return false;
	mod = floating ? imaster : nc - imaster;
	offset = floating ? 0 : imaster;
	relsrc = floating ? cpos : cpos - imaster;
	reldst = (unsigned) (relsrc + (size_t) ((signed) mod + dir))
	         % (unsigned) mod;
	dst = reldst + (unsigned) offset;
	if (npos != NULL)
		*npos = dst;
	if (n != NULL)
		*n = clients[dst];
	return true;
}

void
print(FILE *f, enum log_level level, char const *filename, unsigned line,
      char const *format, ...)
{
	va_list args;
	time_t rawtime;
	struct tm *date;
	char const *col;

	if (level > log_level)
		return;

	/* application name & timestamp */
	rawtime = time(NULL);
	date = localtime(&rawtime);
	(void) fprintf(f, APPNAME" [%04d-%02d-%02d %02d:%02d:%02d] ",
	               date->tm_year+1900, date->tm_mon, date->tm_mday,
	               date->tm_hour, date->tm_min, date->tm_sec);

	/* log level */
	switch (level) {
	case LOG_FATAL : col = "\033[31mFATAL\033[0m "; break;
	case LOG_ERROR : col = "\033[31mERROR\033[0m "; break;
	case LOG_WARN  : col = "\033[33mWARN\033[0m " ; break;
	case LOG_NOTICE: col = "\033[1mNOTICE\033[0m "; break;
	case LOG_EVENT : col = "\033[35mEVENT\033[0m "; break;
	case LOG_DEBUG : col = "\033[34mDEBUG\033[0m "; break;
	default        : col = "";
	}
	(void) fprintf(f, "%s", col);

	/* position */
	if (level >= LOG_EVENT)
		(void) fprintf(f, "\033[32m%s:%u\033[0m ", filename, line);

	/* message */
	va_start(args, format);
	(void) vfprintf(f, format, args);
	va_end(args);

	(void) fprintf(f, "\n");
	(void) fflush(f);
}

static void
restack(void)
{
	unsigned i, i_fl, i_tl, nfloating;
	struct client *c, *restacked[nc];

	for (i = nfloating = 0; i < nc; ++i)
		nfloating += clients[i]->floating;
	for (i = i_fl = i_tl = 0; i < nc; ++i) {
		c = clients[i];
		restacked[c->floating ? i_fl++ : nfloating + i_tl++] = c;
	}
	for (i = 0; i < nc; ++i)
		clients[i] = restacked[i];
	imaster = nfloating;
}

static void
run(void)
{
	XEvent xe;

	running = true;
	while (running && !XNextEvent(kwm.dpy, &xe)) {
		//DEBUG("run(): e.type = %d", xe.type);
		if (handle[xe.type] != NULL)
			handle[xe.type](&xe);
	}
}

/* TODO move XSelectInput to client */
void
setclientmask(bool set)
{
	unsigned i;

	if (set)
		for (i = 0; i < nc; ++i)
			XSelectInput(kwm.dpy, clients[i]->win, CLIENTMASK);
	else
		for (i = 0; i < nc; ++i)
			XSelectInput(kwm.dpy, clients[i]->win, 0);
}

static void
setupsession(void)
{
	unsigned i, nwins;
	Window p, r, *wins = NULL;
	struct client *c;

	/* scan existing windows */
	if (!XQueryTree(kwm.dpy, root, &r, &p, &wins, &nwins)) {
		WARN("XQueryTree() failed");
		return;
	}

	/* create clients if they do not already exist */
	for (i = 0; i < nwins; i++) {
		c = client_new(wins[i]);
		if (c != NULL) {
			NOTICE("detected existing window %lu", c->win);
			attachclient(c);
		}
	}

	/* setup monitors */
	updategeom();
}

static void
sigchld(signed s)
{
	(void) s;

	if (signal(SIGCHLD, sigchld) == SIG_ERR)
		DIE("could not install SIGCHLD handler");
	/* pid -1 makes it equivalent to wait() (wait for all children);
	 * here we just add WNOHANG */
	while (0 < waitpid(-1, NULL, WNOHANG));
}

static void
term(void)
{
	struct client *c;

	while (nc > 0) {
		c = clients[0];
		detachclient(c);
		client_delete(c);
	}

	/* remove X resources */
	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, root);
	XFreeCursor(kwm.dpy, cursor[CurNormal]);
	XFreeCursor(kwm.dpy, cursor[CurResize]);
	XFreeCursor(kwm.dpy, cursor[CurMove]);
	XSetInputFocus(kwm.dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XCloseDisplay(kwm.dpy);
}

static void
updatefocus(void)
{
	unsigned i;

	if (nc == 0) {
		XSetInputFocus(kwm.dpy, root, RevertToPointerRoot, CurrentTime);
		return;
	}

	for (i = 0; i < nc; ++i)
		client_set_focus(clients[i], clients[i] == selcli);
}

static void
updategeom(void)
{
	sw = (unsigned) DisplayWidth(kwm.dpy, screen);
	sh = (unsigned) DisplayHeight(kwm.dpy, screen);
}

signed
main(signed argc, char **argv)
{
	(void) argv;

	log_level = LOG_DEBUG;

	if (argc > 1) {
		puts(APPNAME" © 2015 ayekat, see LICENSE for details");
		return EXIT_FAILURE;
	}
	init();
	run();
	term();
	VERBOSE("shutting down");
	return EXIT_SUCCESS;
}
