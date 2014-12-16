#include "karuiwm.h"
#include "util.h"
#include "client.h"

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
#define APPVERSION "0.1"
#define BUTTONMASK (ButtonPressMask|ButtonReleaseMask)
#define CLIENTMASK (EnterWindowMask|PropertyChangeMask|StructureNotifyMask)
#define INTERSECT(MON, X, Y, W, H) \
	((MAX(0, MIN((X)+(W),(MON)->x+(MON)->w) - MAX((MON)->x, X))) * \
	 (MAX(0, MIN((Y)+(H),(MON)->y+(MON)->h) - MAX((MON)->y, Y))))
#define MOUSEMASK (BUTTONMASK|PointerMotionMask)
#define FATAL(...) print(stderr, LOG_FATAL, __VA_ARGS__)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLAST };
enum { Left, Right, Up, Down, NoDirection };

/* functions */
static struct client *locate_window2client(Window);
static void attachclient(struct client *);
static void buttonpress(XEvent *);
static bool checksizehints(struct client *, int unsigned *, int unsigned *);
static void cleanup(void);
static void clientmessage(XEvent *);
static void configurerequest(XEvent *);
static void configurenotify(XEvent *);
static void detachclient(struct client *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void focusin(XEvent *);
static void grabkeys(void);
static Pixmap initpixmap(long const *, long const, long const);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void killclient(union argument const *);
static void mappingnotify(XEvent *);
static void maprequest(XEvent *);
static void movemouse(union argument const *);
static void propertynotify(XEvent *);
static void quit(union argument const *);
static void resizeclient(struct client *, int unsigned, int unsigned);
static void resizemouse(union argument const *);
static void rstack(void);
static void run(void);
static bool sendevent(struct client *, Atom);
static void setclientmask(bool);
static void setfloating(struct client *, bool);
static void setfullscreen(struct client *, bool);
static void setmfact(union argument const *);
static void setnmaster(union argument const *);
static void setup(void);
static void setupatoms(void);
static void setupfont(void);
static void setupsession(void);
static void shiftclient(union argument const *);
static void sigchld(int);
static void spawn(union argument const *);
static void stepfocus(union argument const *);
static void termclient(struct client *);
static void togglefloat(union argument const *);
static void unmapnotify(XEvent *);
static void updateclient(struct client *);
static void updatefocus(void);
static void updategeom(void);
static void updatetransient(struct client *);
static int xerror(Display *, XErrorEvent *);
static int (*xerrorxlib)(Display *, XErrorEvent *);
static void zoom(union argument const *);

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

/* variables */
static bool running;                          /* application state */
static int screen;                            /* screen */
static Window root;                           /* root window */
static Cursor cursor[CurLAST];                /* cursors */
static int dmenu_out;                         /* dmenu output file descriptor */
static enum log_level log_level;
static struct client **clients, *selcli;
static size_t nc, nmaster;
static float mfact;
static struct monitor *mon;
static int unsigned sw, sh;

/* karuiwm configuration */

#define FONTSTR "-misc-fixed-medium-r-semicondensed--13-100-100-100-c-60-iso8859-1"
#define BORDERWIDTH 1    /* window border width */
#define WSMBOXWIDTH 90   /* WSM box width */
#define WSMBOXHEIGHT 60  /* WSM box height */
#define WSMBORDERWIDTH 2 /* WSM box border width */
#define SESSIONFILE "/tmp/"APPNAME /* file for saving session when restarting */

/* colours */
#define CBORDERNORM      0x222222   /* normal windows */
#define CBORDERSEL       0xAFD700   /* selected windows */

#define CNORM            0x888888   /* status bar */
#define CBGNORM          0x222222

#define CSEL             0xCCCCCC
#define CBGSEL           0x444444

/* commands */
static char const *termcmd[] = { "urxvt", NULL };
static char const *scrotcmd[] = { "prtscr", NULL };
static char const *lockcmd[] = { "scrock", NULL };
static char const *volupcmd[] = { "amixer", "set", "Master", "2+", "unmute", NULL };
static char const *voldowncmd[] = { "amixer", "set", "Master", "2-", "unmute", NULL };
static char const *volmutecmd[] = { "amixer", "set", "Master", "toggle", NULL };

#define MODKEY Mod1Mask

/* normal keys */
static struct key const keys[] = {
	/* applications */
	{ MODKEY,                       XK_n,       spawn,            { .v=termcmd } },
	{ MODKEY,                       XK_Print,   spawn,            { .v=scrotcmd } },

	/* hardware */
	{ 0,                            0x1008FF11, spawn,            { .v=voldowncmd } },
	{ 0,                            0x1008FF12, spawn,            { .v=volmutecmd } },
	{ 0,                            0x1008FF13, spawn,            { .v=volupcmd } },

	/* windows */
	{ MODKEY,                       XK_j,       stepfocus,        { .i=+1 } },
	{ MODKEY,                       XK_k,       stepfocus,        { .i=-1 } },
	{ MODKEY,                       XK_l,       setmfact,         { .f=+0.02 } },
	{ MODKEY,                       XK_h,       setmfact,         { .f=-0.02 } },
	{ MODKEY,                       XK_t,       togglefloat,      { 0 } },
	{ MODKEY|ShiftMask,             XK_c,       killclient,       { 0 } },

	/* layout */
	{ MODKEY|ShiftMask,             XK_j,       shiftclient,      { .i=+1 } },
	{ MODKEY|ShiftMask,             XK_k,       shiftclient,      { .i=-1 } },
	{ MODKEY,                       XK_comma,   setnmaster,       { .i=+1 } },
	{ MODKEY,                       XK_period,  setnmaster,       { .i=-1 } },
	{ MODKEY,                       XK_Return,  zoom,             { 0 } },

	/* session */
	{ MODKEY,                       XK_z,       spawn,            { .v=lockcmd } },
	{ MODKEY|ControlMask|ShiftMask, XK_q,       quit,             { 0 } },
};

/* WSM keys */
static struct key const wsmkeys[] = {
	/* applications */
	{ MODKEY,                       XK_Print,   spawn,            { .v=scrotcmd } },

	/* hardware */
	{ 0,                            0x1008FF11, spawn,            { .v=voldowncmd } },
	{ 0,                            0x1008FF12, spawn,            { .v=volmutecmd } },
	{ 0,                            0x1008FF13, spawn,            { .v=volupcmd } },
};

/* mouse buttons */
static struct button const buttons[] = {
	{ MODKEY,                       Button1,    movemouse,        { 0 } },
	{ MODKEY,                       Button3,    resizemouse,      { 0 } },
};


/* function implementation */

static struct client *
locate_window2client(Window win)
{
	int unsigned i;

	for (i = 0; i < nc; ++i)
		if (clients[i]->win == win)
			return clients[i];
	return NULL;
}

void
attachclient(struct client *c)
{
	unsigned int i, pos;

	clients = srealloc(clients, ++nc*sizeof(struct client *),"client list");
	clients[nc-1] = c;
	selcli = c;
}

void
buttonpress(XEvent *e)
{
	unsigned int i;
	struct client *c;

	c = locate_window2client(e->xbutton.window);
	if (c == NULL) {
		WARN("click on unhandled window");
		return;
	}

	/* update focus */
	selcli = c;
	updatefocus();

	for (i = 0; i < LENGTH(buttons); i++) {
		if (buttons[i].mod == e->xbutton.state
		&& buttons[i].button == e->xbutton.button && buttons[i].func) {
			buttons[i].func(&buttons[i].arg);
		}
	}
}

bool
checksizehints(struct client *c, int unsigned *w, int unsigned *h)
{
	int u;
	bool change = false;

	/* don't respect size hints for tiled or fullscreen windows */
	if (!c->floating || c->fullscreen) {
		return *w != c->w || *h != c->h;
	}

	if (*w != c->w) {
		if (c->basew > 0 && c->incw > 0) {
			u = (*w-c->basew)/c->incw;
			*w = c->basew+u*c->incw;
		}
		if (c->minw > 0) {
			*w = MAX(*w, c->minw);
		}
		if (c->maxw > 0) {
			*w = MIN(*w, c->maxw);
		}
		if (*w != c->w) {
			change = true;
		}
	}
	if (*h != c->h) {
		if (c->baseh > 0 && c->inch > 0) {
			u = (*h-c->baseh)/c->inch;
			*h = c->baseh+u*c->inch;
		}
		if (c->minh > 0) {
			*h = MAX(*h, c->minh);
		}
		if (c->maxh > 0) {
			*h = MIN(*h, c->maxh);
		}
		if (*h != c->h) {
			change = true;
		}
	}
	return change;
}

void
cleanup(void)
{
	unsigned int i;
	struct client *c;

	while (nc > 0) {
		c = clients[0];
		detachclient(c);
		termclient(c);
	}

	/* remove X resources */
	if (dc.font.xfontset) {
		XFreeFontSet(kwm.dpy, dc.font.xfontset);
	} else {
		XFreeFont(kwm.dpy, dc.font.xfontstruct);
	}
	XFreeGC(kwm.dpy, dc.gc);
	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, root);
	XFreeCursor(kwm.dpy, cursor[CurNormal]);
	XFreeCursor(kwm.dpy, cursor[CurResize]);
	XFreeCursor(kwm.dpy, cursor[CurMove]);
	XSetInputFocus(kwm.dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
	XCloseDisplay(kwm.dpy);
}

void
clientmessage(XEvent *e)
{
	DEBUG("clientmessage(%lu)", e->xclient.window);

	int unsigned i;
	struct client *c;
	XClientMessageEvent *ev = &e->xclient;

	c = locate_window2client(e->xclient.window);
	if (c == NULL) {
		WARN("client message on unhandled window");
		return;
	}

	if (ev->message_type == netatom[NetWMState]) {
		if (ev->data.l[1] == netatom[NetWMStateFullscreen] ||
				ev->data.l[2] == netatom[NetWMStateFullscreen]) {
			setfullscreen(c, ev->data.l[0] == 1 || /* _NET_WM_STATE_ADD */
					(ev->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ &&
					 !c->fullscreen));
		}
	}
}

void
configurenotify(XEvent *e)
{
	DEBUG("configurenotify(%lu)", e->xconfigure.window);
	if (e->xconfigure.window == root) {
		updategeom();
	}
}

void
configurerequest(XEvent *e)
{
	int unsigned i;
	struct client *c = NULL;
	XWindowChanges wc;
	XConfigureEvent cev;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;

	DEBUG("configurerequest(%lu)", e->xconfigurerequest.window);

	/* forward configuration if not managed (or if we don't force the size) */
	c = locate_window2client(e->xconfigurerequest.window);
	if (c == NULL || c->fullscreen || c->floating) {
		wc = (XWindowChanges) {
			.x = ev->x,
			.y = ev->y,
			.width = ev->width,
			.height = ev->height,
			.border_width = ev->border_width,
			.sibling = ev->above,
			.stack_mode = ev->detail
		};
		XConfigureWindow(kwm.dpy, ev->window, ev->value_mask, &wc);
		return;
	}

	/* force size with XSendEvent() instead of ordinary XConfigureWindow() */
	cev = (XConfigureEvent) {
		.type = ConfigureNotify,
		.display = kwm.dpy,
		.event = c->win,
		.window = c->win,
		.x = c->x,
		.y = c->y,
		.width = c->w,
		.height = c->h,
		.border_width = BORDERWIDTH,
		.above = None,
		.override_redirect = False,
	};
	XSendEvent(kwm.dpy, c->win, false, StructureNotifyMask, (XEvent *) &cev);
}

void
detachclient(struct client *c)
{
	int unsigned i;

	for (i = 0; i < nc && clients[i] != c; ++i);
	if (i == nc) {
		WARN("attempt to detach non-existent client");
		return;
	}

	/* update focus */
	selcli = nc > 0 ? clients[0] : NULL;

	/* remove from list, shift */
	nc--;
	for (; i < nc; i++) {
		clients[i] = clients[i+1];
	}
	clients = srealloc(clients, nc*sizeof(struct client *),
	                   "client list");
}

void
enternotify(XEvent *e)
{
	DEBUG("enternotify(%lu)", e->xcrossing.window);

	unsigned int pos;
	struct client *c;

	c = locate_window2client(e->xcrossing.window);
	if (c == NULL) {
		WARN("attempt to enter unhandled/invisible window %u",
		     e->xcrossing.window);
		return;
	}
	selcli = c;
	updatefocus();
}

void
expose(XEvent *e)
{
	DEBUG("expose(%lu)", e->xexpose.window);
	/* TODO */
}

/* fix for VTE terminals that tend to steal the focus */
void
focusin(XEvent *e)
{
	Window win = e->xfocus.window;

	if (win == root) {
		return;
	}
	if (nc == 0 || win != selcli->win) {
		WARN("focusin on unfocused window %d (focus is on %d)",
		      e->xfocus.window, nc > 0 ? selcli->win : -1);
		updatefocus();
	}
}

void
grabbuttons(struct client *c, bool focused)
{
	unsigned int i;

	XUngrabButton(kwm.dpy, AnyButton, AnyModifier, c->win);
	if (focused) {
		for (i = 0; i < LENGTH(buttons); i++) {
			XGrabButton(kwm.dpy, buttons[i].button, buttons[i].mod, c->win, false,
					BUTTONMASK, GrabModeAsync, GrabModeAsync, None, None);
		}
	} else {
		XGrabButton(kwm.dpy, AnyButton, AnyModifier, c->win, false, BUTTONMASK,
				GrabModeAsync, GrabModeAsync, None, None);
	}
}

void
grabkeys(void)
{
	unsigned int i;

	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, root);
	for (i = 0; i < LENGTH(keys); i++) {
		XGrabKey(kwm.dpy, XKeysymToKeycode(kwm.dpy, keys[i].key), keys[i].mod, root,
				true, GrabModeAsync, GrabModeAsync);
	}
}

Pixmap
initpixmap(long const *bitfield, long const fg, long const bg)
{
	int x, y;
	long w = bitfield[0];
	long h = bitfield[1];
	Pixmap icon;

	icon = XCreatePixmap(kwm.dpy, root, w, h, dc.sd);
	XSetForeground(kwm.dpy, dc.gc, bg);
	XFillRectangle(kwm.dpy, icon, dc.gc, 0, 0, w, h);
	XSetForeground(kwm.dpy, dc.gc, fg);
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			if ((bitfield[y+2]>>(w-x-1))&1) {
				XDrawPoint(kwm.dpy, icon, dc.gc, x, y);
			}
		}
	}
	return icon;
}

void
keypress(XEvent *e)
{
	//debug("keypress()");

	unsigned int i;
	KeySym keysym = XLookupKeysym(&e->xkey, 0);

	for (i = 0; i < LENGTH(keys); i++) {
		if (e->xkey.state == keys[i].mod && keysym == keys[i].key &&
				keys[i].func) {
			keys[i].func(&keys[i].arg);
			return;
		}
	}
}

void
keyrelease(XEvent *e)
{
	//debug("keyrelease()");
}

void
killclient(union argument const *arg)
{
	struct client *c;
	(void) arg;

	/* nothing to kill */
	if (nc == 0) {
		return;
	}

	/* try to kill the client via the WM_DELETE_WINDOW atom */
	c = selcli;
	if (sendevent(c, wmatom[WMDeleteWindow])) {
		return;
	}

	/* otherwise massacre the client */
	XGrabServer(kwm.dpy);
	XSetCloseDownMode(kwm.dpy, DestroyAll);
	XKillClient(kwm.dpy, selcli->win);
	XSync(kwm.dpy, false);
	XUngrabServer(kwm.dpy);
}

void
maprequest(XEvent *e)
{
	DEBUG("maprequest(%lu)", e->xmaprequest.window);

	struct client *c;
	Window win = e->xmaprequest.window;

	/* in case a window gets remapped */
	c = locate_window2client(win);
	if (c != NULL) {
		detachclient(c);
		termclient(c);
	}
	c = client_init(win, false);
	if (c == NULL)
		return;

	/* display */
	attachclient(c);
	rstack();
	XMapWindow(kwm.dpy, c->win);

	/* update client information (fullscreen/dialog/transient) and focus */
	updateclient(c);
	updatetransient(c);
	updatefocus();
}

void
mappingnotify(XEvent *e)
{
	DEBUG("mappingnotify(%lu)", e->xmapping.window);
	/* TODO */
}

void
movemouse(union argument const *arg)
{
	struct monitor *mon;
	XEvent ev;
	Window dummy;
	struct client *c = selcli;
	int cx=c->x, cy=c->y, x, y, i;
	unsigned int ui;
	(void) arg;

	DEBUG("movemouse(%lu)", c->win);

	/* don't move fullscreen client */
	if (c->fullscreen) {
		return;
	}

	/* grab the pointer and change the cursor appearance */
	if (XGrabPointer(kwm.dpy, root, true, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, cursor[CurMove], CurrentTime) != GrabSuccess) {
		WARN("XGrabPointer() failed");
		return;
	}

	/* get initial pointer position */
	if (!XQueryPointer(kwm.dpy, root, &dummy, &dummy, &x, &y, &i, &i, &ui)) {
		WARN("XQueryPointer() failed");
		return;
	}

	/* handle motions */
	do {
		XMaskEvent(kwm.dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch (ev.type) {
			case ConfigureRequest:
			case Expose:
			case MapRequest:
				handle[ev.type](&ev);
				break;
			case MotionNotify:
				if (!c->floating) {
					togglefloat(NULL);
				}
				cx = cx+(ev.xmotion.x-x);
				cy = cy+(ev.xmotion.y-y);
				x = ev.xmotion.x;
				y = ev.xmotion.y;
				client_move(c, cx, cy);
				break;
		}
	} while (ev.type != ButtonRelease);
	XUngrabPointer(kwm.dpy, CurrentTime);
}

void
moveresizeclient(struct client *c, int x, int y, int unsigned w, int unsigned h)
{
	client_move(c, x, y);
	resizeclient(c, w, h);
}

void
print(FILE *f, enum log_level level, char const *format, ...)
{
	va_list args;
	time_t rawtime;
	struct tm *date;
	char const *col;

	if (level > log_level)
		return;

	/* application name & timestamp */
	time(&rawtime);
	date = localtime(&rawtime);
	(void) fprintf(f, APPNAME" [%04d-%02d-%02d|%02d:%02d:%02d] ",
	               date->tm_year+1900, date->tm_mon, date->tm_mday,
	               date->tm_hour, date->tm_min, date->tm_sec);

	/* log level */
	switch (level) {
	case LOG_FATAL: col = "\033[31mFATAL\033[0m "; break;
	case LOG_ERROR: col = "\033[31mERROR\033[0m "; break;
	case LOG_WARN : col = "\033[33mWARN\033[0m " ; break;
	default       : col = "";
	}
	(void) fprintf(f, "%s ", col);

	/* message */
	va_start(args, format);
	(void) vfprintf(f, format, args);
	va_end(args);

	(void) fprintf(f, "\n");
	(void) fflush(f);
}

void
propertynotify(XEvent *e)
{
	DEBUG("propertynotify(%lu)", e->xproperty.window);

	XPropertyEvent *ev;
	struct client *c;

	ev = &e->xproperty;
	c = locate_window2client(ev->window);
	if (c == NULL)
		return;
	switch (ev->atom) {
		case XA_WM_TRANSIENT_FOR:
			DEBUG("\033[33mwindow %lu is transient!\033[0m", c->win);
			updatetransient(c);
			break;
		case XA_WM_NORMAL_HINTS:
			client_updatesizehints(c);
			break;
		case XA_WM_HINTS:
			/* TODO urgent hint */
			break;
	}
	if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
		client_updatename(c);
	}
	if (ev->atom == netatom[NetWMWindowType]) {
		updateclient(c);
	}
}

void
quit(union argument const *arg)
{
	(void) arg;

	running = false;
}

void
resizeclient(struct client *c, int unsigned w, int unsigned h)
{
	if (checksizehints(c, &w, &h))
		XResizeWindow(kwm.dpy, c->win, c->w = MAX(w, 1), c->h = MAX(h, 1));
}

void
resizemouse(union argument const *arg)
{
	XEvent ev;
	struct client *c = selcli;
	int x, y;
	unsigned int cw=c->w, ch=c->h;
	(void) arg;

	DEBUG("resizemouse(%lu)", c->win);

	/* don't resize fullscreen or dialog client */
	if (c->fullscreen || c->dialog) {
		WARN("attempt to resize fullscreen client");
		return;
	}

	/* grab the pointer and change the cursor appearance */
	if (XGrabPointer(kwm.dpy, root, false, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, cursor[CurResize], CurrentTime) != GrabSuccess) {
		WARN("XGrabPointer() failed");
		return;
	}

	/* set pointer position to lower right */
	XWarpPointer(kwm.dpy, None, c->win, 0, 0, 0, 0,
			c->w+2*BORDERWIDTH-1, c->h+2*BORDERWIDTH-1);
	x = c->x+c->w+2*BORDERWIDTH-1;
	y = c->y+c->h+2*BORDERWIDTH-1;

	/* handle motions */
	do {
		XMaskEvent(kwm.dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		//debug("\033[38;5;238m[resizemouse] run(): e.type=%d\033[0m", ev.type);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handle[ev.type](&ev);
			break;
		case MotionNotify:
			if (!c->floating) {
				togglefloat(NULL);
			}
			cw = cw+(ev.xmotion.x_root-x);
			ch = ch+(ev.xmotion.y_root-y);
			x = ev.xmotion.x_root;
			y = ev.xmotion.y_root;
			resizeclient(c, cw, ch);
			break;
		}
	} while (ev.type != ButtonRelease);

	XUngrabPointer(kwm.dpy, CurrentTime);
}

void
restart(union argument const *arg)
{
	(void) arg;

	running = false;
}

static void
rstack(void)
{
	unsigned int i, ncm, w, h;
	int x;

	setclientmask(false);

	/* draw master area */
	ncm = MIN(nmaster, nc);
	if (ncm) {
		x = 0;
		w = ncm == nc ? sw : mfact*sw;
		h = sh/ncm;
		for (i = 0; i < ncm; i++) {
			moveresizeclient(clients[i], x, i*h,
					w-2*BORDERWIDTH, h-2*BORDERWIDTH);
		}
	}
	if (ncm == nc) {
		return;
	}

	/* draw stack area */
	x = ncm > 0 ? mfact*sw : 0;
	w = ncm > 0 ? sw-x : sw;
	h = sh/(nc-ncm);
	for (i = ncm; i < nc; i++) {
		moveresizeclient(clients[i], x, (i-ncm)*h,
				w-2*BORDERWIDTH, h-2*BORDERWIDTH);
	}

	setclientmask(true);
}

void
run(void)
{
	XEvent e;
	int ret;
	fd_set fds;
	struct timeval timeout = { .tv_usec = 0, .tv_sec = 0 };
	dmenu_out =

	running = true;
	while (running && !XNextEvent(kwm.dpy, &e)) {
		//debug("\033[38;5;238mrun(): e.type=%d\033[0m", e.type);
		if (handle[e.type]) {
			handle[e.type](&e);
		}
	}
}

bool
sendevent(struct client *c, Atom atom)
{
	int n;
	Atom *supported;
	bool exists;
	XEvent ev;

	if (XGetWMProtocols(kwm.dpy, c->win, &supported, &n)) {
		while (!exists && n--) {
			exists = supported[n] == atom;
		}
		XFree(supported);
	}
	if (!exists) {
		return false;
	}
	ev.type = ClientMessage;
	ev.xclient.window = c->win;
	ev.xclient.message_type = wmatom[WMProtocols];
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = atom;
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(kwm.dpy, c->win, false, NoEventMask, &ev);
	return true;
}

void
setclientmask(bool set)
{
	int i;

	if (set) {
		for (i = 0; i < nc; i++) {
			XSelectInput(kwm.dpy, clients[i]->win, CLIENTMASK);
		}
	} else {
		for (i = 0; i < nc; i++) {
			XSelectInput(kwm.dpy, clients[i]->win, 0);
		}
	}
}

void
setfloating(struct client *c, bool floating)
{

	if (floating) {
		XRaiseWindow(kwm.dpy, c->win);
	} else {
		XLowerWindow(kwm.dpy, c->win);
	}
	c->floating = floating;
	rstack();
	updatefocus();
}

void
setfullscreen(struct client *c, bool fullscreen)
{
	c->fullscreen = fullscreen;
	if (fullscreen) {
		c->oldx = c->x;
		c->oldy = c->y;
		c->oldw = c->w;
		c->oldh = c->h;
		XSetWindowBorderWidth(kwm.dpy, c->win, 0);
		moveresizeclient(c, 0, 0, sw, sh);
		XRaiseWindow(kwm.dpy, c->win);
	} else {
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		XSetWindowBorderWidth(kwm.dpy, c->win, BORDERWIDTH);
		if (!c->floating) {
			rstack();
		}
	}
}

void
setmfact(union argument const *arg)
{
	mfact = MAX(0.1, MIN(0.9, mfact+arg->f));
	rstack();
}

void
setnmaster(union argument const *arg)
{
	nmaster = MAX(0, nmaster + arg->i);
	rstack();
}

void
setup(void)
{
	XSetWindowAttributes wa;

	/* connect to X */
	if (!(kwm.dpy = XOpenDisplay(NULL))) {
		DIE("could not open X");
	}

	/* low: errors, zombies, locale */
	xerrorxlib = XSetErrorHandler(xerror);
	sigchld(0);
	if (!setlocale(LC_ALL, "") || !XSupportsLocale()) {
		FATAL("could not set locale");
		exit(EXIT_FAILURE);
	}

	/* basic: root window, graphic context, atoms */
	screen = DefaultScreen(kwm.dpy);
	root = RootWindow(kwm.dpy, screen);
	dc.gc = XCreateGC(kwm.dpy, root, 0, NULL);
	dc.sd = DefaultDepth(kwm.dpy, screen);
	setupatoms();

	/* events */
	wa.event_mask = SubstructureNotifyMask|SubstructureRedirectMask|
			KeyPressMask|StructureNotifyMask|ButtonPressMask|PointerMotionMask|
			FocusChangeMask|PropertyChangeMask;
	XChangeWindowAttributes(kwm.dpy, root, CWEventMask, &wa);

	/* input: mouse, keyboard */
	cursor[CurNormal] = XCreateFontCursor(kwm.dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(kwm.dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(kwm.dpy, XC_fleur);
	wa.cursor = cursor[CurNormal];
	XChangeWindowAttributes(kwm.dpy, root, CWCursor, &wa);
	grabkeys();

	/* layouts, fonts, workspace map, scratchpad, dmenu */
	setupfont();

	/* session */
	setupsession();
}

void
setupatoms(void)
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

void
setupfont(void)
{
	XFontStruct **xfonts;
	char **xfontnames;
	char *def, **missing;
	int n;

	dc.font.xfontset = XCreateFontSet(kwm.dpy, FONTSTR, &missing, &n, &def);
	if (missing) {
		while (n--) {
			WARN("missing fontset: %s", missing[n]);
		}
		XFreeStringList(missing);
	}

	/* if fontset load is successful, get information; otherwise dummy font */
	if (dc.font.xfontset) {
		dc.font.ascent = dc.font.descent = 0;
		n = XFontsOfFontSet(dc.font.xfontset, &xfonts, &xfontnames);
		while (n--) {
			dc.font.ascent = MAX(dc.font.ascent, xfonts[n]->ascent);
			dc.font.descent = MAX(dc.font.descent, xfonts[n]->descent);
		}
	} else {
		dc.font.xfontstruct = XLoadQueryFont(kwm.dpy, FONTSTR);
		if (!dc.font.xfontstruct) {
			FATAL("cannot load font '%s'", FONTSTR);
			exit(EXIT_FAILURE);
		}
		dc.font.ascent = dc.font.xfontstruct->ascent;
		dc.font.descent = dc.font.xfontstruct->descent;
	}
	dc.font.height = dc.font.ascent + dc.font.descent;
}

void
setupsession(void)
{
	unsigned int i, j, nwins;
	Window p, r, *wins = NULL;
	struct client *c;

	DEBUG("setupsession:1");
	DEBUG("setupsession:2");
	/* scan existing windows */
	if (!XQueryTree(kwm.dpy, root, &r, &p, &wins, &nwins)) {
		WARN("XQueryTree() failed");
		return;
	}

	DEBUG("setupsession:3 » nwins = %u", nwins);
	/* create clients if they do not already exist */
	for (i = 0; i < nwins; i++) {
		DEBUG("setupsession:3.1 » wins[%u] = %lu", i, wins[i]);
		c = client_init(wins[i], true);
		if (c != NULL) {
			WARN("scanned unhandled window %lu", c->win);
			attachclient(c);
		}
	}

	DEBUG("setupsession:4");
	/* setup monitors */
	updategeom();
	DEBUG("setupsession:5");
}

void
shiftclient(union argument const *arg)
{
	int unsigned pos;
	(void) arg;

	if (nc < 2) {
		return;
	}
	for (pos = 0; pos < nc && clients[pos] != selcli; ++pos);
	clients[pos] = clients[(pos+nc+arg->i)%nc];
	clients[(pos+nc+arg->i)%nc] = selcli;
	rstack();
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR) {
		FATAL("could not install SIGCHLD handler");
		exit(EXIT_FAILURE);
	}
	/* pid -1 makes it equivalent to wait() (wait for all children);
	 * here we just add WNOHANG */
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(union argument const *arg)
{
	pid_t pid = fork();
	if (pid == 0) {
		execvp(((char const **)arg->v)[0], (char **)arg->v);
		ERROR("execvp(%s) failed: %s",
		      ((char const **)arg->v)[0], strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid < 0) {
		WARN("fork() failed with code %d", pid);
	}
}

void
stepfocus(union argument const *arg)
{
	unsigned int pos;

	if (nc < 2) {
		return;
	}
	for (pos = 0; pos < nc && clients[pos] != selcli; ++pos);
	selcli = clients[(pos+nc+arg->i)%nc];
	updatefocus();
	if (selcli->floating) {
		XRaiseWindow(kwm.dpy, selcli->win);
	}
}

void
termclient(struct client *c)
{
	free(c);
}

void
togglefloat(union argument const *arg)
{
	(void) arg;

	if (selcli == NULL) {
		return;
	}
	setfloating(selcli, !selcli->floating);
}

void
unmapnotify(XEvent *e)
{
	DEBUG("unmapnotify(%lu)", e->xunmap.window);

	struct client *c;

	locate_window2client(e->xunmap.window);
	if (c == NULL) {
		return;
	}

	detachclient(c);
	termclient(c);
	rstack();
}

void
updateclient(struct client *c)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da;

	if (XGetWindowProperty(kwm.dpy, c->win, netatom[NetWMState], 0, sizeof(da),
			false, XA_ATOM, &da, &di, &dl, &dl, &p) == Success && p) {
		if ((Atom) *p == netatom[NetWMStateFullscreen]) {
			setfullscreen(c, true);
		}
		free(p);
	}
	if (XGetWindowProperty(kwm.dpy, c->win, netatom[NetWMWindowType], 0, sizeof(da),
			false, XA_ATOM, &da, &di, &dl, &dl, &p) == Success && p) {
		if ((Atom) *p == netatom[NetWMWindowTypeDialog]) {
			DEBUG("\033[32mwindow %lu is a dialog!\033[0m", c->win);
			c->dialog = true;
			setfloating(c, true);
		} else {
			DEBUG("\033[32mwindow %lu is not a dialog\033[0m", c->win);
		}
	} else {
		DEBUG("\033[32mcould not get window property for %lu\033[0m", c->win);
	}
}

void
updatefocus(void)
{
	int unsigned i;
	struct client *c;

	if (nc == 0) {
		XSetInputFocus(kwm.dpy, root, RevertToPointerRoot, CurrentTime);
		return;
	}

	for (i = 0; i < nc; ++i) {
		c = clients[i];
		if (selcli == c) {
			XSetWindowBorder(kwm.dpy, c->win, CBORDERSEL);
			XSetInputFocus(kwm.dpy, root, RevertToPointerRoot, CurrentTime);
			grabbuttons(c, true);
		} else {
			XSetWindowBorder(kwm.dpy, c->win, CBORDERNORM);
			grabbuttons(c, false);
		}
	}
}

void
updategeom(void)
{
	sw = DisplayWidth(kwm.dpy, screen);
	sh = DisplayHeight(kwm.dpy, screen);
}

void
updatetransient(struct client *c)
{
	Window trans = 0;

	if (!XGetTransientForHint(kwm.dpy, c->win, &trans)) {
		DEBUG("window %lu is not transient", c->win);
		return;
	}
	setfloating(c, true);
}

int
xerror(Display *dpy, XErrorEvent *ee)
{
	char es[256];
	XGetErrorText(dpy, ee->error_code, es, 256);
	ERROR("%s (ID %d) after request %d", es, ee->error_code, ee->error_code);
	/* default error handler (will likely call exit) */
	return xerrorxlib(dpy, ee);
}

void
zoom(union argument const *arg)
{
	unsigned int pos;
	(void) arg;

	if (nc < 2) {
		return;
	}

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
	rstack();
}

int
main(int argc, char **argv)
{
	(void) argc;
	(void) argv;

	log_level = LOG_DEBUG;

	if (argc > 1) {
		puts(APPNAME" © 2014 ayekat, see LICENSE for details");
		return EXIT_FAILURE;
	}
	setup();
	run();
	cleanup();
	return EXIT_SUCCESS;
}
