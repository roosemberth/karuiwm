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

/* functions (TODO prepend with action_ and event_ accordingly) */
static void arrange(void);
static void attachclient(struct client *c);
static void buttonpress(XEvent *xe);
static void clientmessage(XEvent *xe);
static void configurerequest(XEvent *xe);
static void configurenotify(XEvent *xe);
static void detachclient(struct client *c);
static void enternotify(XEvent *xe);
static void expose(XEvent *xe);
static void focusin(XEvent *xe);
static void grabkeys(void);
static void init(void);
static void init_atoms(void);
static void keypress(XEvent *xe);
static void keyrelease(XEvent *xe);
static void killclient(union argument const *arg);
static void mappingnotify(XEvent *xe);
static void maprequest(XEvent *xe);
static void propertynotify(XEvent *xe);
static void quit(union argument const *arg);
static void restack(size_t ntiled, struct client **tiled,
                    size_t nfloating, struct client **floating,
                    size_t nfullscreen, struct client **fullscreen);
static void run(void);
static void setmfact(union argument const *arg);
static void setnmaster(union argument const *arg);
static void setupsession(void);
static void shiftclient(union argument const *arg);
static void sigchld(int);
static void spawn(union argument const *arg);
static void stepfocus(union argument const *arg);
static void term(void);
static void togglefloat(union argument const *arg);
static void unmapnotify(XEvent *xe);
static void updatefocus(void);
static void updategeom(void);
static int xerror(Display *dpy, XErrorEvent *xe);
static int (*xerrorxlib)(Display *dpy, XErrorEvent *xe);
static void zoom(union argument const *arg);

/* variables */
static bool running;                          /* application state */
static int screen;                            /* screen */
static Window root;                           /* root window */
static Cursor cursor[CurLAST];                /* cursors */
static enum log_level log_level;
static struct client **clients, *selcli;
static size_t nc, nmaster = 1;
static float mfact = 0.5;
static int unsigned sw, sh;
static char const *termcmd[] = { "urxvt", NULL };
static char const *scrotcmd[] = { "prtscr", NULL };
static char const *volupcmd[] = { "amixer", "set", "Master", "2+", "unmute", NULL };
static char const *voldowncmd[] = { "amixer", "set", "Master", "2-", "unmute", NULL };
static char const *volmutecmd[] = { "amixer", "set", "Master", "toggle", NULL };

/* TODO hack (see comment in client.c) */
static struct button const buttons[] = {
	{ MODKEY,           Button1,    movemouse,   { 0 } },
};

/* event handlers, as array to allow O(1) access; codes in X.h */
static void (*handle[LASTEvent])(XEvent *) = {
	[ButtonPress]      = buttonpress,      /* 4*/
	[ClientMessage]    = clientmessage,    /*33*/
	[ConfigureNotify]  = configurenotify,  /*22*/
	[ConfigureRequest] = configurerequest, /*23*/
	[EnterNotify]      = enternotify,      /* 7*/
	[Expose]           = expose,           /*12*/
	[FocusIn]          = focusin,          /* 9*/
	[KeyPress]         = keypress,         /* 2*/
	[KeyRelease]       = keyrelease,       /* 3*/
	[MapRequest]       = maprequest,       /*20*/
	[MappingNotify]    = mappingnotify,    /*34*/
	[PropertyNotify]   = propertynotify,   /*28*/
	[UnmapNotify]      = unmapnotify       /*18*/
};

/* keys */
static struct key const keys[] = {
	/* applications */
	{ MODKEY,           XK_n,       spawn,       { .v=termcmd } },
	{ MODKEY,           XK_Print,   spawn,       { .v=scrotcmd } },

	/* hardware */
	{ 0,                0x1008FF11, spawn,       { .v=voldowncmd } },
	{ 0,                0x1008FF12, spawn,       { .v=volmutecmd } },
	{ 0,                0x1008FF13, spawn,       { .v=volupcmd } },

	/* windows */
	{ MODKEY,           XK_j,       stepfocus,   { .i=+1 } },
	{ MODKEY,           XK_k,       stepfocus,   { .i=-1 } },
	{ MODKEY,           XK_l,       setmfact,    { .f=+0.02f } },
	{ MODKEY,           XK_h,       setmfact,    { .f=-0.02f } },
	{ MODKEY,           XK_t,       togglefloat, { 0 } },
	{ MODKEY|ShiftMask, XK_c,       killclient,  { 0 } },

	/* layout */
	{ MODKEY|ShiftMask, XK_j,       shiftclient, { .i=+1 } },
	{ MODKEY|ShiftMask, XK_k,       shiftclient, { .i=-1 } },
	{ MODKEY,           XK_comma,   setnmaster,  { .i=+1 } },
	{ MODKEY,           XK_period,  setnmaster,  { .i=-1 } },
	{ MODKEY,           XK_Return,  zoom,        { 0 } },

	/* session */
	{ MODKEY|ShiftMask, XK_q,       quit,        { 0 } },
};

/* implementation */
static void
arrange(void)
{
	int unsigned i;
	size_t ntiled = 0, nfloating = 0, nfullscreen = 0;
	struct client *tiled[nc], *floating[nc], *fullscreen[nc], *c;

	if (nc == 0)
		return;

	/* categorise: fullscreen, floating, tiled */
	for (i = 0; i < nc; ++i) {
		c = clients[i];
		if (c->fullscreen)
			fullscreen[nfullscreen++] = c;
		else if (c->floating)
			floating[nfloating++] = c;
		else
			tiled[ntiled++] = c;
	}

	setclientmask(false);

	/* arrange */
	if (ntiled > 0) {
		/* TODO modular layout management */
		rstack(tiled, ntiled, MIN(nmaster, ntiled), mfact, sw, sh);
	}
	for (i = 0; i < nfloating; ++i) {
		c = floating[i];
		client_moveresize(c, c->x, c->y, c->w, c->h);
	}
	for (i = 0; i < nfullscreen; ++i) {
		c = fullscreen[i];
		client_moveresize(c, 0, 0, sw, sh);
	}

	/* TODO not necessary: whoever needs restack should do it explicitely */
	restack(ntiled, tiled, nfloating, floating, nfullscreen, fullscreen);

	setclientmask(true);
}

static void
attachclient(struct client *c)
{
	attach((void ***) &clients, &nc, c, sizeof(c), "client list");
	selcli = c;
}

static void
buttonpress(XEvent *xe)
{
	unsigned int i;
	struct client *c;
	XButtonEvent *e = &xe->xbutton;

	if (!locate_window2client(clients, nc, e->window, &c, NULL)) {
		WARN("click on unhandled window");
		return;
	}

	/* update focus */
	selcli = c;
	updatefocus();

	for (i = 0; i < LENGTH(buttons); i++) {
		if (buttons[i].mod == e->state
		&& buttons[i].button == e->button && buttons[i].func) {
			buttons[i].func(&buttons[i].arg);
		}
	}
}

static void
clientmessage(XEvent *xe)
{
	bool fullscreen;
	struct client *c;
	XClientMessageEvent *e = &xe->xclient;

	//EVENT("clientmessage(%lu)", e->window);

	/* check override_redirect */
	if (!locate_window2client(clients, nc, e->window, &c, NULL))
		return;

	if (e->message_type == netatom[NetWMState]) {
		if ((Atom) e->data.l[1] == netatom[NetWMStateFullscreen]
		|| (Atom) e->data.l[2] == netatom[NetWMStateFullscreen]) {
			/* _NET_WM_STATE_ADD || _NET_WM_STATE_TOGGLE */
			fullscreen = e->data.l[0] == 1 ||
			             (e->data.l[0] == 2 && !c->fullscreen);
			client_setfullscreen(c, fullscreen);
			arrange();
		}
	}
}

static void
configurenotify(XEvent *xe)
{
	XConfigureEvent *e = &xe->xconfigure;

	//EVENT("configurenotify(%lu)", e->window);

	if (e->window == root)
		updategeom();
}

static void
configurerequest(XEvent *xe)
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
		                 (int unsigned) e->value_mask, &wc);
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
	cev.width = (int) c->w;
	cev.height = (int) c->h;
	cev.border_width = BORDERWIDTH;
	cev.above = None;
	cev.override_redirect = False;
	XSendEvent(kwm.dpy, c->win, False, StructureNotifyMask, (XEvent *) &cev);
}

static void
detachclient(struct client *c)
{
	int pos = detach((void ***) &clients, &nc, c, sizeof(struct client),
	                 "client list");
	if (pos < 0) {
		WARN("attempt to detach unhandled client(%lu)", c->win);
		return;
	}
	selcli = nc > 0 ? clients[MIN((int unsigned) pos, nc - 1)] : NULL;
}

static void
enternotify(XEvent *xe)
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
expose(XEvent *xe)
{
	XExposeEvent *e = &xe->xexpose;

	//EVENT("expose(%lu)", e->window);
	(void) e;

	/* TODO */
}

/* fix for VTE terminals that tend to steal the focus */
static void
focusin(XEvent *xe)
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
grabkeys(void)
{
	int unsigned i;

	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, root);
	for (i = 0; i < LENGTH(keys); i++)
		XGrabKey(kwm.dpy, XKeysymToKeycode(kwm.dpy, keys[i].key),
		         keys[i].mod, root, True, GrabModeAsync, GrabModeAsync);
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
	xerrorxlib = XSetErrorHandler(xerror);
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

static void
keypress(XEvent *xe)
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
keyrelease(XEvent *xe)
{
	XKeyReleasedEvent *e = &xe->xkey;
	(void) e;

	//EVENT("keyrelease()");

	/* TODO */
}

static void
killclient(union argument const *arg)
{
	(void) arg;

	if (selcli != NULL)
		client_kill(selcli);
}

static void
mappingnotify(XEvent *xe)
{
	XMappingEvent *e = &xe->xmapping;

	//EVENT("mappingnotify(%lu)", e->window);
	(void) e;

	/* TODO */
}

static void
maprequest(XEvent *xe)
{
	struct client *c = NULL;
	XMapRequestEvent *e = &xe->xmaprequest;

	//EVENT("maprequest(%lu)", e->window);

	/* in case of remap */
	if (locate_window2client(clients, nc, e->window, &c, NULL)) {
		detachclient(c);
		client_term(c);
	}

	/* initialise */
	c = client_init(e->window);
	if (c == NULL)
		return;

	/* fix dimension */
	if (c->floating)
		client_moveresize(c, MAX(c->x, 0), MAX(c->y, 0),
		                     MIN(c->w, sw), MIN(c->h, sh));

	XMapWindow(kwm.dpy, c->win);

	attachclient(c);
	arrange();
	updatefocus();
}

/* TODO split up */
void
movemouse(union argument const *arg)
{
	XEvent ev;
	Window w;
	struct client *c = selcli;
	int cx = c->x, cy = c->y, x, y, i;
	int unsigned ui;
	int long evmask = MOUSEMASK | ExposureMask | SubstructureRedirectMask;
	(void) arg;

	//EVENT("movemouse(%lu)", c->win);

	/* don't move fullscreen client */
	if (c->fullscreen)
		return;

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
			if (!c->floating)
				togglefloat(NULL);
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

void
print(FILE *f, enum log_level level, char const *filename, int unsigned line,
      char const *format, ...)
{
	va_list args;
	time_t rawtime;
	struct tm *date;
	char const *col;

	if (level > log_level)
		return;

	/* application name & timestamp */
	(void) time(&rawtime);
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
	if (level == LOG_DEBUG)
		(void) fprintf(f, "\033[32m%s:%u\033[0m ", filename, line);

	/* message */
	va_start(args, format);
	(void) vfprintf(f, format, args);
	va_end(args);

	(void) fprintf(f, "\n");
	(void) fflush(f);
}

static void
propertynotify(XEvent *xe)
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
		client_querytransient(c);
		break;
	case XA_WM_NORMAL_HINTS:
		DEBUG("size hints changed for window %lu", c->win);
		client_querysizehints(c);
		break;
	case XA_WM_HINTS:
		DEBUG("urgent hint changed for window %u", c->win);
		/* TODO */
		break;
	}
	if (e->atom == XA_WM_NAME || e->atom == netatom[NetWMName])
		client_queryname(c);
	if (e->atom == netatom[NetWMWindowType]) {
		client_queryfullscreen(c);
		client_querydialog(c);
	}
	arrange();
}

static void
quit(union argument const *arg)
{
	(void) arg;

	running = false;
}

static void
restack(size_t ntiled, struct client **tiled,
        size_t nfloating, struct client **floating,
        size_t nfullscreen, struct client **fullscreen)
{
	int unsigned i, m = 0;
	Window wins[nc];

	for (i = 0; i < nfullscreen; ++i)
		wins[m++] = fullscreen[i]->win;
	for (i = 0; i < nfloating; ++i)
		wins[m++] = floating[i]->win;
	for (i = 0; i < ntiled; ++i)
		wins[m++] = tiled[i]->win;
	XRestackWindows(kwm.dpy, wins, (int) nc);
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
	int unsigned i;

	if (set)
		for (i = 0; i < nc; ++i)
			XSelectInput(kwm.dpy, clients[i]->win, CLIENTMASK);
	else
		for (i = 0; i < nc; ++i)
			XSelectInput(kwm.dpy, clients[i]->win, 0);
}

static void
setmfact(union argument const *arg)
{
	mfact = MAX(0.1f, MIN(0.9f, mfact + arg->f));
	arrange();
}

static void
setnmaster(union argument const *arg)
{
	if (arg->i < 0 && (size_t) (-arg->i) > nmaster)
		return;
	nmaster = (size_t) ((int) nmaster + arg->i);
	arrange();
}

static void
setupsession(void)
{
	unsigned int i, nwins;
	Window p, r, *wins = NULL;
	struct client *c;

	/* scan existing windows */
	if (!XQueryTree(kwm.dpy, root, &r, &p, &wins, &nwins)) {
		WARN("XQueryTree() failed");
		return;
	}

	/* create clients if they do not already exist */
	for (i = 0; i < nwins; i++) {
		c = client_init(wins[i]);
		if (c != NULL) {
			NOTICE("detected existing window %lu", c->win);
			attachclient(c);
		}
	}

	/* setup monitors */
	updategeom();
}

static void
shiftclient(union argument const *arg)
{
	int unsigned pos;
	(void) arg;

	if (nc < 2) {
		return;
	}
	for (pos = 0; pos < nc && clients[pos] != selcli; ++pos);
	clients[pos] = clients[(pos + (size_t) ((int) nc + arg->i))%nc];
	clients[(pos + (size_t) ((int) nc + arg->i))%nc] = selcli;
	arrange();
}

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

static void
spawn(union argument const *arg)
{
	char *const *cmd = (char *const *) arg->v;

	pid_t pid = fork();
	if (pid == 0) {
		execvp(cmd[0], cmd);
		ERROR("execvp(%s) failed: %s", cmd[0], strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid < 0) {
		WARN("fork() failed with code %d", pid);
	}
}

static void
stepfocus(union argument const *arg)
{
	int unsigned pos;

	if (nc < 2) {
		return;
	}
	for (pos = 0; pos < nc && clients[pos] != selcli; ++pos);
	selcli = clients[(pos + (size_t) ((int) nc + arg->i))%nc];
	updatefocus();
	if (selcli->floating)
		/* TODO restack instead (see TODO in arrange) */
		XRaiseWindow(kwm.dpy, selcli->win);
}

static void
term(void)
{
	struct client *c;

	while (nc > 0) {
		c = clients[0];
		detachclient(c);
		client_term(c);
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
togglefloat(union argument const *arg)
{
	(void) arg;

	if (selcli == NULL)
		return;
	client_setfloating(selcli, !selcli->floating);
	arrange();
}

static void
unmapnotify(XEvent *xe)
{
	struct client *c;
	XUnmapEvent *e = &xe->xunmap;

	//EVENT("unmapnotify(%lu)", e->window);

	if (!locate_window2client(clients, nc, e->window, &c, NULL))
		return;

	detachclient(c);
	client_term(c);
	updatefocus();
	arrange();
}

static void
updatefocus(void)
{
	int unsigned i;

	if (nc == 0) {
		XSetInputFocus(kwm.dpy, root, RevertToPointerRoot, CurrentTime);
		return;
	}

	for (i = 0; i < nc; ++i)
		client_setfocus(clients[i], clients[i] == selcli);
}

static void
updategeom(void)
{
	sw = (int unsigned) DisplayWidth(kwm.dpy, screen);
	sh = (int unsigned) DisplayHeight(kwm.dpy, screen);
}

static int
xerror(Display *dpy, XErrorEvent *ee)
{
	char s[256];

	XGetErrorText(dpy, ee->error_code, s, 256);
	ERROR("%s (ID %d) after request %d", s, ee->error_code, ee->error_code);

	/* call default error handler (very likely to exit) */
	return xerrorxlib(dpy, ee);
}

static void
zoom(union argument const *arg)
{
	int unsigned pos;
	(void) arg;

	if (nc < 2)
		return;

	for (pos = 0; pos < nc && clients[pos] != selcli; ++pos);

	if (pos == 0) {
		/* window is at the top: swap with next below */
		clients[0] = clients[1];
		clients[1] = selcli;
		selcli = clients[0];
		updatefocus();
	} else {
		/* window is somewhere else: swap with top */
		clients[pos] = clients[0];
		clients[0] = selcli;
	}
	arrange();
}

int
main(int argc, char **argv)
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
	return EXIT_SUCCESS;
}
