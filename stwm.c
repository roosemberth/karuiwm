#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xinerama.h>

/* macros */
#define APPNAME "stwm"
#define BUTTONMASK (ButtonPressMask|ButtonReleaseMask)
#define CLIENTMASK (EnterWindowMask)
#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MOUSEMASK (BUTTONMASK|PointerMotionMask)

/* log macros */
#define warn(...) stdlog(stderr, "\033[33mWRN\033[0m "__VA_ARGS__)
#define die(...) warn("\033[31mERR\033[0m "__VA_ARGS__); exit(EXIT_FAILURE)
#define note(...) stdlog(stdout, "    "__VA_ARGS__)
#ifdef DEBUG
#define debug(...) stdlog(stdout, "\033[34mDBG\033[0m "__VA_ARGS__)
#else
#define debug(...)
#endif

/* enums */
enum { CURSOR_NORMAL, CURSOR_RESIZE, CURSOR_MOVE, CURSOR_LAST };
enum { LEFT, RIGHT, UP, DOWN, NO_DIRECTION };

typedef struct Workspace Workspace;

typedef union {
	int i;
	float f;
	const void *v;
} Arg;

typedef struct {
	unsigned int mod;
	unsigned int button;
	void (*func)(Arg const *);
	Arg const arg;
} Button;

typedef struct {
	int x, y;
	unsigned int w, h;
	char name[256];
	Window win;
	bool floating;
	int basew, baseh, incw, inch;
} Client;

typedef struct {
	GC gc;
	struct {
		int ascent, descent, height;
		XFontStruct *xfontstruct;
		XFontSet xfontset;
	} font;
} DC;

typedef struct {
	unsigned int mod;
	KeySym key;
	void (*func)(Arg const *);
	Arg const arg;
} Key;

typedef struct {
	Workspace *selws;
	unsigned int x, y, w, h;     /* monitor dimensions */
	unsigned int bx, by, bw, bh; /* status bar dimensions */
	unsigned int wx, wy, ww, wh; /* workspace dimensions */
	struct {
		Window win;
		char buffer[256];
		unsigned int cursor;
	} bar;
} Monitor;

struct Workspace {
	Client **clients, **stack;
	Client *selcli;
	Window wsdbox;
	unsigned int nc, ns;
	unsigned int nmaster;
	float mfact;
	char name[256];
	int x, y;
};

typedef struct {
	Workspace *target;
	XSetWindowAttributes wa;
	int w, h; /* size of a box */
	int rows, cols, rad;
	bool active;
	XIM im;
	XIC ic;
} WorkspaceDialog;

/* functions */
static void arrange(Monitor *);
static void attachclient(Workspace *, Client *);
static void attachws(Workspace *);
static void buttonpress(XEvent *);
static bool checksizehints(Client *, int, int, unsigned int *, unsigned int *);
static void cleanup(void);
static void clientmessage(XEvent *);
static bool collision(Workspace *);
static void configurerequest(XEvent *);
static void configurenotify(XEvent *);
static void detachclient(Client *);
static void detachmon(Monitor *);
static void detachws(Workspace *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void grabbuttons(Client *, bool);
static void grabkeys(void);
static void hide(Client *);
static void hidews(Workspace *);
static void initbar(Monitor *);
static Client *initclient(Window, bool);
static Monitor *initmon(void);
static Workspace *initws(int, int);
static Window initwsdbox(void);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static char keytoansi(KeySym *);
static void killclient(Arg const *);
static bool locateclient(Workspace **, Client **, unsigned int *, Window const);
static bool locatemon(Monitor **, unsigned int *, Workspace const *);
static bool locatews(Workspace **, unsigned int *, int, int, char const *);
static void mappingnotify(XEvent *);
static void maprequest(XEvent *);
static void moveclient(Arg const *);
static void movemouse(Arg const *);
static void moveresize(Client *, int, int, unsigned int, unsigned int);
static void movews(Arg const *);
static void pop(Workspace *, Client *);
static void propertynotify(XEvent *);
static void push(Workspace *, Client *);
static void quit(Arg const *);
static void reltoxy(int *, int *, Workspace *, int);
static void renamews(Workspace *, char const *);
static void renderbar(Monitor *);
static void renderwsdbox(Workspace *);
static void resizemouse(Arg const *);
static void restart(Arg const *);
static void run(void);
static void scan(void);
static void setmfact(Arg const *);
static void setup(void);
static void setupfont(void);
static void setupwsd(void);
static void setnmaster(Arg const *);
static void setws(int, int);
static void shift(Arg const *);
static void sigchld(int);
static void spawn(Arg const *);
static void stdlog(FILE *, char const *, ...);
static void stepfocus(Arg const *);
static void stepmon(Arg const *);
static void stepws(Arg const *);
static void stepwsdbox(Arg const *arg);
static void termclient(Client *);
static void termmon(Monitor *);
static void termws(Workspace *);
static unsigned int textwidth(char const *, unsigned int);
static void tile(Monitor *);
static void togglefloat(Arg const *);
static void togglewsd(Arg const *);
static size_t unifyscreens(XineramaScreenInfo **, size_t);
static void unmapnotify(XEvent *);
static void updatebar(Monitor *);
static void updatefocus(void);
static void updategeom(void);
static void updatemon(Monitor *, int, int, unsigned int, unsigned int);
static void updatesizehints(Client *);
static void updatewsdmap(void);
static void updatewsdbar(XEvent *, char const *);
static void updatewsdbox(Workspace *);
static int xerror(Display *, XErrorEvent *);
static int (*xerrorxlib)(Display *, XErrorEvent *);
static void zoom(Arg const *);

/* event handlers, as array to allow O(1) access; codes in X.h */
static void (*handle[LASTEvent])(XEvent *) = {
	[ButtonPress]      = buttonpress,      /* 4*/
	[ClientMessage]    = clientmessage,    /*33*/
	[ConfigureNotify]  = configurenotify,  /*22*/
	[ConfigureRequest] = configurerequest, /*23*/
	[EnterNotify]      = enternotify,      /* 7*/
	[Expose]           = expose,           /*12*/
	[KeyPress]         = keypress,         /* 2*/
	[KeyRelease]       = keyrelease,       /* 3*/
	[MapRequest]       = maprequest,       /*20*/
	[MappingNotify]    = mappingnotify,    /*34*/
	[PropertyNotify]   = propertynotify,   /*28*/
	[UnmapNotify]      = unmapnotify       /*18*/
};

/* variables */
static bool running, restarting;    /* application state */
static Display *dpy;                /* X display */
static int screen;                  /* screen */
static Window root;                 /* root window */
static Workspace **workspaces;      /* list of workspaces */
static unsigned int nws;            /* number of workspaces */
static DC dc;                       /* drawing context */
static Cursor cursor[CURSOR_LAST];  /* cursors */
static WorkspaceDialog wsd;         /* workspace dialog */
static Monitor **monitors;          /* list of monitors */
static Monitor *selmon;             /* selected monitor */
static unsigned int nmon;           /* number of monitors */

/* configuration */
#include "config.h"

void
arrange(Monitor *mon)
{
	unsigned int i;
	for (i = 0; i < mon->selws->nc; i++) {
		XSelectInput(dpy, mon->selws->clients[i]->win, 0);
	}
	/* TODO use array of layouts */
	tile(mon);
	for (i = 0; i < mon->selws->nc; i++) {
		XSelectInput(dpy, mon->selws->clients[i]->win, CLIENTMASK);
	}
}

void
attachclient(Workspace *ws, Client *c)
{
	unsigned int i, pos;

	/* add to list */
	ws->clients = realloc(ws->clients, ++ws->nc*sizeof(Client *));
	if (!ws->clients) {
		die("could not allocate %u bytes for client list",
				ws->nc*sizeof(Client *));
	}
	pos = 0;
	if (ws->nc > 1) {
		for (i = 0; i < ws->nc-1; i++) {
			if (ws->clients[i] == ws->selcli) {
				pos = i+1;
				for (i = ws->nc-1; i > pos; i--) {
					ws->clients[i] = ws->clients[i-1];
				}
				break;
			}
		}
	}
	ws->clients[pos] = c;

	/* add to stack */
	push(ws, c);

	/* update layout if on a selected workspace */
	for (i = 0; i < nmon; i++) {
		if (monitors[i]->selws == ws) {
			arrange(monitors[i]);
			break;
		}
	}
}

void
attachmon(Monitor *mon)
{
	monitors = realloc(monitors, ++nmon*sizeof(Monitor *));
	if (!monitors) {
		die("could not allocate %u bytes for monitor list", sizeof(Monitor));
	}
	monitors[nmon-1] = mon;
}

void
attachws(Workspace *ws)
{
	workspaces = realloc(workspaces, ++nws*sizeof(Workspace *));
	if (!workspaces) {
		die("could not allocate %u bytes for workspace list",
				nws*sizeof(Workspace *));
	}
	workspaces[nws-1] = ws;
	renamews(ws, NULL);
}

void
buttonpress(XEvent *e)
{
	unsigned int i;
	Client *c;
	Workspace *ws;

	if (!locateclient(&ws, &c, NULL, e->xbutton.window)) {
		return;
	}
	push(ws, c);
	updatefocus();

	for (i = 0; i < LENGTH(buttons); i++) {
		if (buttons[i].mod == e->xbutton.state &&
				buttons[i].button == e->xbutton.button && buttons[i].func) {
			buttons[i].func(&buttons[i].arg);
		}
	}
}

bool
checksizehints(Client *c, int x, int y, unsigned int *w, unsigned int *h)
{
	unsigned int u;
	bool change = false;;

	if ((!c->floating && FORCESIZE) || (!c->basew && !c->baseh)) {
		return true;
	}

	if (*w != c->w) {
		u = (*w-c->basew)/c->incw;
		*w = c->basew+u*c->incw;
		if (*w != c->w) {
			change = true;
		}
	}
	if (*h != c->h) {
		u = (*h-c->baseh)/c->inch;
		*h = c->baseh+u*c->inch;
		if (*h != c->h) {
			change = true;
		}
	}
	return change || c->x != x || c->y != y;
}

void
cleanup(void)
{
	unsigned int i;
	Monitor *mon;
	Workspace *ws;
	Client *c;

	/* disable WSD */
	if (wsd.active) {
		togglewsd(NULL);
	}

	/* make monitors point nowhere (so all workspaces are removed) */
	for (i = 0; i < nmon; i++) {
		monitors[i]->selws = NULL;
	}

	/* remove workspaces and their clients */
	while (nws) {
		ws = workspaces[0];
		while (ws->nc) {
			c = ws->clients[0];
			detachclient(c);
			termclient(c);
		}
		if (nws && ws == workspaces[0]) { /* workspace is still here */
			detachws(ws);
			termws(ws);
		}
	}
	termws(wsd.target);

	/* remove monitors */
	while (nmon) {
		mon = monitors[0];
		detachmon(mon);
		termmon(mon);
	}

	/* graphic context */
	XFreeGC(dpy, dc.gc);
}

void
clientmessage(XEvent *e)
{
	debug("clientmessage(%d)", e->xclient.window);
	/* TODO */
}

bool
collision(Workspace *ws)
{
	Monitor *mon;
	return (locatemon(&mon, NULL, ws) && mon != selmon);
}

void
configurenotify(XEvent *e)
{
	debug("configurenotify(%d)", e->xconfigure.window);
	if (e->xconfigure.window == root) {
		updategeom();
	}
}

void
configurerequest(XEvent *e)
{
	debug("configurerequest(%d)", e->xconfigurerequest.window);

	Client *c = NULL;
	XWindowChanges wc;
	XConfigureEvent cev;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;

	/* forward configuration if not managed (or if we don't force the size) */
	if (!FORCESIZE || !locateclient(NULL, &c, NULL, ev->window) || c->floating) {
		wc = (XWindowChanges) {
			.x = ev->x,
			.y = ev->y,
			.width = ev->width,
			.height = ev->height,
			.border_width = ev->border_width,
			.sibling = ev->above,
			.stack_mode = ev->detail
		};
		XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
		return;
	}

	/* force size with XSendEvent() instead of ordinary XConfigureWindow() */
	cev = (XConfigureEvent) {
		.type = ConfigureNotify,
		.display = dpy,
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
	XSendEvent(dpy, c->win, false, StructureNotifyMask, (XEvent *) &cev);
}

void
detachclient(Client *c)
{
	unsigned int i;
	Workspace *ws;
	Monitor *mon;

	if (!locateclient(&ws, &c, &i, c->win)) {
		warn("attempt to detach an unhandled window %d", c->win);
		return;
	}

	/* remove from stack */
	pop(ws, c);

	/* remove from list */
	ws->nc--;
	for (; i < ws->nc; i++) {
		ws->clients[i] = ws->clients[i+1];
	}
	ws->clients = realloc(ws->clients, ws->nc*sizeof(Client *));
	if (!ws->clients && ws->nc) {
		die ("could not allocate %u bytes for client list",
				ws->nc*sizeof(Client *));
	}

	/* update layout if selected workspace; otherwise remove if last client  */
	if (locatemon(&mon, NULL, ws)) {
		arrange(mon);
		updatefocus();
	} else if (!ws->nc) {
		detachws(ws);
		termws(ws);
	}
}

void
detachmon(Monitor *mon)
{
	unsigned int i;

	if (!locatemon(&mon, &i, mon->selws)) {
		warn("attempt to detach non-existing monitor");
		return;
	}
	for (nmon--; i < nmon; i++) {
		monitors[i] = monitors[i+1];
	}
	monitors = realloc(monitors, nmon*sizeof(Monitor *));
}

void
detachws(Workspace *ws)
{
	unsigned int i;

	if (!locatews(&ws, &i, ws->x, ws->y, NULL)) {
		warn("attempt to detach non-existing workspace");
		return;
	}

	nws--;
	for (; i < nws; i++) {
		workspaces[i] = workspaces[i+1];
	}
	workspaces = realloc(workspaces, nws*sizeof(Workspace *));
	if (!workspaces && nws) {
		die("could not allocate %u bytes for workspace list",
				nws*sizeof(Workspace *));
	}

	if (ws->wsdbox) {
		XDestroyWindow(dpy, ws->wsdbox);
		ws->wsdbox = 0;
		updatewsdmap();
	}
}

void
enternotify(XEvent *e)
{
	debug("enternotify(%d)", e->xcrossing.window);

	unsigned int pos;
	Workspace *ws;
	Monitor *mon;
	Client *c;

	if (!locateclient(&ws, &c, &pos, e->xcrossing.window)) {
		warn("attempt to enter unhandled/invisible window %d",
				e->xcrossing.window);
		return;
	}
	if (locatemon(&mon, NULL, ws) && mon != selmon) {
		selmon = mon;
	}
	push(selmon->selws, c);
	updatefocus();
}

void
expose(XEvent *e)
{
	debug("expose(%d)", e->xexpose.window);

	unsigned int i;

	/* status bar */
	for (i = 0; i < nmon; i++) {
		if (e->xexpose.window == monitors[i]->bar.win) {
			renderbar(monitors[i]);
		}
	}

	/* WSD boxes */
	for (i = 0; i < nws; i++) {
		if (e->xexpose.window == workspaces[i]->wsdbox) {
			renderwsdbox(workspaces[i]);
		}
	}
	if (e->xexpose.window == wsd.target->wsdbox) {
		renderwsdbox(wsd.target);
	}
}

void
grabbuttons(Client *c, bool focused)
{
	unsigned int i;

	XUngrabButton(dpy, AnyButton, AnyModifier, c->win);
	if (focused) {
		for (i = 0; i < LENGTH(buttons); i++) {
			XGrabButton(dpy, buttons[i].button, buttons[i].mod, c->win, false,
					BUTTONMASK, GrabModeAsync, GrabModeAsync, None, None);
		}
	} else {
		XGrabButton(dpy, AnyButton, AnyModifier, c->win, false, BUTTONMASK,
				GrabModeAsync, GrabModeAsync, None, None);
	}
}

void
grabkeys(void)
{
	unsigned int i;

	XUngrabKey(dpy, AnyKey, AnyModifier, root);
	for (i = 0; i < LENGTH(keys); i++) {
		XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].key), keys[i].mod, root,
				true, GrabModeAsync, GrabModeAsync);
	}
}

void
hide(Client *c)
{
	XMoveWindow(dpy, c->win, -c->w-2*BORDERWIDTH, c->y);
}

void
hidews(Workspace *ws)
{
	unsigned int i;
	for (i = 0; i < ws->nc; i++) {
		hide(ws->clients[i]);
	}
}

void
initbar(Monitor *mon)
{
	XSetWindowAttributes wa;

	mon->bh = dc.font.height + 2;
	mon->bar.buffer[0] = 0;
	mon->bar.cursor = 0;

	wa.override_redirect = true;
	wa.background_pixmap = ParentRelative;
	wa.event_mask = ExposureMask;
	mon->bar.win = XCreateWindow(dpy, root, 0, -mon->bh, 10, mon->bh, 0,
			DefaultDepth(dpy, screen), CopyFromParent,
			DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	XMapWindow(dpy, mon->bar.win);
}

Client *
initclient(Window win, bool viewable)
{
	Client *c;
	XWindowAttributes wa;

	/* ignore buggy windows or windows with override_redirect */
	if (!XGetWindowAttributes(dpy, win, &wa)) {
		warn("XGetWindowAttributes() failed for window %d", win);
		return NULL;
	}
	if (wa.override_redirect) {
		return NULL;
	}

	/* ignore unviewable windows if we request for viewable windows */
	if (viewable && wa.map_state != IsViewable) {
		return NULL;
	}

	/* create client */
	c = malloc(sizeof(Client));
	if (!c) {
		die("could not allocate %u bytes for client", sizeof(Client));
	}
	c->win = win;
	c->floating = false; /* TODO apply rules */
	if (c->floating) {
		c->x = wa.x;
		c->y = wa.y;
		c->w = wa.width;
		c->h = wa.height;
	}
	XSetWindowBorderWidth(dpy, c->win, BORDERWIDTH);
	updatesizehints(c);
	grabbuttons(c, false);
	XSelectInput(dpy, c->win, CLIENTMASK);
	return c;
}

Monitor *
initmon(void)
{
	unsigned int wsx;
	Monitor *mon;
	Workspace *ws;

	mon = malloc(sizeof(Monitor));
	if (!mon) {
		die("could not allocate %u bytes for monitor", sizeof(Monitor));
	}

	/* assign workspace */
	for (wsx = 0;; wsx++) {
		if (!locatews(&ws, NULL, wsx, 0, NULL)) {
			mon->selws = initws(wsx, 0);
			attachws(mon->selws);
			break;
		} else if (!locatemon(NULL, NULL, ws)) {
			mon->selws = ws;
			break;
		}
	}

	/* add status bar */
	initbar(mon);

	return mon;
}

Workspace *
initws(int x, int y)
{
	Workspace *ws = malloc(sizeof(Workspace));
	if (!ws) {
		die("could not allocate %u bytes for workspace", sizeof(Workspace));
	}
	ws->clients = ws->stack = NULL;
	ws->nc = ws->ns = 0;
	ws->x = x;
	ws->y = y;
	ws->mfact = MFACT;
	ws->nmaster = NMASTER;
	ws->name[0] = 0;
	ws->wsdbox = 0;
	return ws;
}

Window
initwsdbox(void)
{
	Window box = XCreateWindow(dpy, root, -wsd.w, -wsd.h,
			wsd.w-2*WSDBORDERWIDTH, wsd.h-2*WSDBORDERWIDTH, 0,
			DefaultDepth(dpy, screen), CopyFromParent,
			DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wsd.wa);
	XMapWindow(dpy, box);
	return box;
}

void
keypress(XEvent *e)
{
	unsigned int i;
	KeySym keysym = XLookupKeysym(&e->xkey, 0);

	/* catch normal keys */
	if (!wsd.active) {
		for (i = 0; i < LENGTH(keys); i++) {
			if (e->xkey.state == keys[i].mod && keysym == keys[i].key &&
					keys[i].func) {
				keys[i].func(&keys[i].arg);
				return;
			}
		}
	}

	/* catch WSD keys */
	for (i = 0; i < LENGTH(wsdkeys); i++) {
		if (e->xkey.state == wsdkeys[i].mod && keysym == wsdkeys[i].key &&
				wsdkeys[i].func) {
			wsdkeys[i].func(&wsdkeys[i].arg);
			updatewsdmap();
			return;
		}
	}

	/* pass rest on to input bar */
	updatewsdbar(e, NULL);
}

void
keyrelease(XEvent *e)
{
	//debug("keyrelease()");
}

char
keytoansi(KeySym *keysym)
{
	switch (*keysym) {
		case XK_Home:      return 0x01; break; /* ^A */
		case XK_Left:      return 0x02; break; /* ^B */
		case XK_Delete:    return 0x04; break; /* ^D */
		case XK_End:       return 0x05; break; /* ^E */
		case XK_Right:     return 0x06; break; /* ^F */
		case XK_BackSpace: return 0x08; break; /* ^H */
		case XK_Tab:       return 0x09; break; /* ^I */
		case XK_Return:    return 0x0D; break; /* ^M */
		case XK_Down:      return 0x0E; break; /* ^N */
		case XK_Up:        return 0x10; break; /* ^P */
		case XK_Escape:    return 0x1B; break; /* ^[ */
		default:           return 0;    /* no match */
	}
}

void
killclient(Arg const *arg)
{
	int n;
	Client *c;
	Atom protocol, request;
	Atom *supported;
	bool match;
	XClientMessageEvent cmev;

	/* nothing to kill */
	if (!selmon->selws->nc) {
		return;
	}

	/* check if we may communicate to the client via atoms */
	protocol = XInternAtom(dpy, "WM_PROTOCOLS", false);
	request = XInternAtom(dpy, "WM_DELETE_WINDOW", false);
	c = selmon->selws->selcli;
	if (XGetWMProtocols(dpy, c->win, &supported, &n)) {
		while (!match && n--) {
			match = supported[n] == request;
		}
		XFree(supported);
	}
	if (match) {
		cmev = (XClientMessageEvent) {
			.type = ClientMessage,
			.window = c->win,
			.message_type = protocol,
			.format = 32,
			.data.l[0] = request,
			.data.l[1] = CurrentTime
		};
		XSendEvent(dpy, c->win, false, NoEventMask, (XEvent *) &cmev);
		return;
	}

	/* fallback if the client does not speak our language */
	XGrabServer(dpy);
	XSetCloseDownMode(dpy, DestroyAll);
	XKillClient(dpy, selmon->selws->selcli->win);
	XSync(dpy, false);
	XUngrabServer(dpy);
}

bool
locateclient(Workspace **ws, Client **c, unsigned int *pos, Window w)
{
	unsigned int i, j;

	for (j = 0; j < nws; j++) {
		for (i = 0; i < workspaces[j]->nc; i++) {
			if (workspaces[j]->clients[i]->win == w) {
				if (ws) *ws = workspaces[j];
				if (c) *c = workspaces[j]->clients[i];
				if (pos) *pos = i;
				return true;
			}
		}
	}
	return false;
}

bool
locatemon(Monitor **mon, unsigned int *pos, Workspace const *ws)
{
	unsigned int i;

	for (i = 0; i < nmon; i++) {
		if (monitors[i]->selws == ws) {
			if (mon) *mon = monitors[i];
			if (pos) *pos = i;
			return true;;
		}
	}
	return false;
}

bool
locatews(Workspace **ws, unsigned int *pos, int x, int y, char const *name)
{
	unsigned int i;
	for (i = 0; i < nws; i++) {
		if (name) {
			if (!strncmp(name, workspaces[i]->name, strlen(name))) {
				if (ws) *ws = workspaces[i];
				if (pos) *pos = i;
				return true;
			}
		} else {
			if (workspaces[i]->x == x && workspaces[i]->y == y) {
				if (ws) *ws = workspaces[i];
				if (pos) *pos = i;
				return true;
			}
		}
	}
	return false;
}

void
maprequest(XEvent *e)
{
	debug("maprequest(%d)", e->xmaprequest.window);

	Client *c = initclient(e->xmap.window, false);
	if (c) {
		attachclient(selmon->selws, c);
		if (c->floating) {
			XMapRaised(dpy, c->win);
		} else {
			XMapWindow(dpy, c->win);
		}
		updatefocus();
	}
}

void
mappingnotify(XEvent *e)
{
	debug("mappingnotify(%d)", e->xmapping.window);
	/* TODO */
}

void
moveclient(Arg const *arg)
{
	Client *c = selmon->selws->selcli;
	detachclient(c);
	stepws(arg);
	attachclient(selmon->selws, c);
	updatefocus();
}

void
movemouse(Arg const *arg)
{
	XEvent ev;
	Window dummy;
	Client *c = selmon->selws->selcli;
	int cx=c->x, cy=c->y, x, y, i;
	unsigned int ui;

	/* grab the pointer and change the cursor appearance */
	if (XGrabPointer(dpy, root, true, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, cursor[CURSOR_MOVE], CurrentTime) != GrabSuccess) {
		warn("XGrabPointer() failed");
		return;
	}

	/* get initial pointer position */
	if (!XQueryPointer(dpy, root, &dummy, &dummy, &x, &y, &i, &i, &ui)) {
		warn("XQueryPointer() failed");
		return;
	}

	/* handle motions */
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
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
				moveresize(c, cx, cy, c->w, c->h);
				break;
		}
	} while (ev.type != ButtonRelease);

	XUngrabPointer(dpy, CurrentTime);
}

void
moveresize(Client *c, int x, int y, unsigned int w, unsigned int h)
{
	if (checksizehints(c, x, y, &w, &h)) {
		XMoveResizeWindow(dpy, c->win, c->x = x, c->y = y,
				c->w = MAX(w, 1), c->h = MAX(h, 1));
	}
}

void
movews(Arg const *arg)
{
	Workspace *src=NULL, *dst=NULL;
	Workspace *ref = wsd.active ? wsd.target : selmon->selws;
	int dx, dy;

	reltoxy(&dx, &dy, ref, arg->i);
	locatews(&src, NULL, ref->x, ref->y, NULL);
	locatews(&dst, NULL, dx, dy, NULL);

	if (dst) {
		dst->x = ref->x;
		dst->y = ref->y;
	}
	if (src) {
		src->x = dx;
		src->y = dy;
	}
	if (wsd.active) {
		wsd.target->x = dx;
		wsd.target->y = dy;
	}
}

void
pop(Workspace *ws, Client *c)
{
	unsigned int i;

	if (!c) {
		warn("attempt to pop NULL client");
		return;
	}

	for (i = 0; i < ws->ns; i++) {
		if (ws->stack[i] == c) {
			ws->ns--;
			for (; i < ws->ns; i++) {
				ws->stack[i] = ws->stack[i+1];
			}
			ws->stack = realloc(ws->stack, ws->ns*sizeof(Client *));
			if (ws->ns && !ws->stack) {
				die("could not allocate %u bytes for stack",
						ws->ns*sizeof(Client*));
			}
			break;
		}
	}
}

void
propertynotify(XEvent *e)
{
	debug("propertynotify(%d)", e->xproperty.window);
	/* TODO */
}

void
push(Workspace *ws, Client *c)
{
	if (!c) {
		warn("attempt to push NULL client");
		return;
	}
	pop(ws, c);
	ws->stack = realloc(ws->stack, ++ws->ns*sizeof(Client *));
	if (!ws->stack) {
		die("could not allocated %u bytes for stack", ws->ns*sizeof(Client *));
	}
	ws->stack[ws->ns-1] = c;
}

void
quit(Arg const *arg)
{
	restarting = false;
	running = false;
}

void
reltoxy(int *x, int *y, Workspace *ws, int direction)
{
	if (x) *x = ws->x;
	if (y) *y = ws->y;
	switch (direction) {
		case LEFT:  if (x) (*x)--; break;
		case RIGHT: if (x) (*x)++; break;
		case UP:    if (y) (*y)--; break;
		case DOWN:  if (y) (*y)++; break;
	}
}

void
renamews(Workspace *ws, char const *name)
{
	unsigned int n, w;
	bool found;

	/* name specified */
	if (name) {
		strncpy(ws->name, name, 255);
		ws->name[255] = 0;
		return;
	}

	/* name not specified, search random name */
	for (n = 0; n < LENGTH(wsnames); n++) {
		found = false;
		for (w = 0; w < nws; w++) {
			if (!strcmp(wsnames[n], workspaces[w]->name)) {
				found = true;
				break;
			}
		}
		if (!found) {
			strcpy(ws->name, wsnames[n]);
			return;
		}
	}
	warn("workspace name pool exhausted");
	strcpy(ws->name, "");
}

void
renderbar(Monitor *mon)
{
	/* TODO X pixel offset */

	/* background */
	XSetForeground(dpy, dc.gc, wsd.active ? CBGSEL : CBGNORM);
	XFillRectangle(dpy, mon->bar.win, dc.gc, 0, 0, mon->bw, mon->bh);

	/* foreground */
	XSetForeground(dpy, dc.gc, wsd.active ? CSEL : CNORM);
	Xutf8DrawString(dpy, mon->bar.win, dc.font.xfontset, dc.gc, 6,
			dc.font.ascent+1, mon->bar.buffer, strlen(mon->bar.buffer));

	/* cursor if WSD */
	if (wsd.active) {
		XFillRectangle(dpy, mon->bar.win, dc.gc,
				textwidth(mon->bar.buffer, mon->bar.cursor)+5, 1,
				1, dc.font.height);
	}
	XSync(dpy, false);
}

void
renderwsdbox(Workspace *ws)
{
	/* border */
	XSetWindowBorderWidth(dpy, ws->wsdbox, WSDBORDERWIDTH);
	XSetWindowBorder(dpy, ws->wsdbox, ws == selmon->selws ? WSDCBORDERSEL
			: ws == wsd.target ? WSDCBORDERTARGET : WSDCBORDERNORM);

	/* background */
	XSetForeground(dpy, dc.gc, ws == selmon->selws ? WSDCBGSEL
			: ws == wsd.target ? WSDCBGTARGET : WSDCBGNORM);
	XFillRectangle(dpy, ws->wsdbox, dc.gc, 0, 0, wsd.w, wsd.h);

	/* text */
	XSetForeground(dpy, dc.gc, ws == selmon->selws ? WSDCSEL
			: ws == wsd.target ? WSDCTARGET : WSDCNORM);
	Xutf8DrawString(dpy, ws->wsdbox, dc.font.xfontset, dc.gc, 2,
			dc.font.ascent+1, ws->name, strlen(ws->name));
}

void
resizemouse(Arg const *arg)
{
	XEvent ev;
	Client *c = selmon->selws->selcli;
	int x, y;
	unsigned int cw=c->w, ch=c->h;

	/* grab the pointer and change the cursor appearance */
	if (XGrabPointer(dpy, root, false, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, cursor[CURSOR_RESIZE], CurrentTime) != GrabSuccess) {
		warn("XGrabPointer() failed");
		return;
	}

	/* set initial pointer position to lower right */
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
			c->w+2*BORDERWIDTH-1, c->h+2*BORDERWIDTH-1);
	x = c->w+2*BORDERWIDTH-1;
	y = c->h+2*BORDERWIDTH-1;

	/* handle motions */
	do {
		XMaskEvent(dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
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
				moveresize(c, c->x, c->y, cw, ch);
				break;
		}
	} while (ev.type != ButtonRelease);

	XUngrabPointer(dpy, CurrentTime);
}

void
restart(Arg const *arg)
{
	restarting = true;
	running = false;
}

void
run(void)
{
	XEvent e;
	running = true;
	while (running && !XNextEvent(dpy, &e)) {
		//debug("run(): e.type=%d", e.type);
		if (handle[e.type]) {
			handle[e.type](&e);
		}
	}
}

void
scan(void)
{
	unsigned int i, nwins;
	Window p, r, *wins = NULL;
	Client *c;

	if (!XQueryTree(dpy, root, &r, &p, &wins, &nwins)) {
		warn("XQueryTree() failed");
		return;
	}
	for (i = 0; i < nwins; i++) {
		c = initclient(wins[i], true);
		if (c) {
			attachclient(selmon->selws, c);
			updatefocus();
		}
	}
}

void
setmfact(Arg const *arg)
{
	selmon->selws->mfact = MAX(0.1, MIN(0.9, selmon->selws->mfact+arg->f));
	arrange(selmon);
}

void
setup(void)
{
	XSetWindowAttributes wa;

	/* low: errors, zombies, locale */
	xerrorxlib = XSetErrorHandler(xerror);
	sigchld(0);
	if (!setlocale(LC_ALL, "") || !XSupportsLocale()) {
		die("could not set locale");
	}

	/* basic: root window, graphic context */
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	dc.gc = XCreateGC(dpy, root, 0, NULL);

	/* mouse and keyboard */
	cursor[CURSOR_NORMAL] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CURSOR_RESIZE] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CURSOR_MOVE] = XCreateFontCursor(dpy, XC_fleur);
	wa.cursor = cursor[CURSOR_NORMAL];
	XChangeWindowAttributes(dpy, root, CWCursor, &wa);
	grabkeys();

	/* event mask */
	wa.event_mask = SubstructureNotifyMask|SubstructureRedirectMask|
			KeyPressMask|StructureNotifyMask|ButtonPressMask|PointerMotionMask;
	XChangeWindowAttributes(dpy, root, CWEventMask, &wa);

	/* font */
	setupfont();

	/* monitors */
	updategeom();

	/* workspace dialog */
	setupwsd();
}

void
setupfont(void)
{
	XFontStruct **xfonts;
	char **xfontnames;
	char *def, **missing;
	int n;

	dc.font.xfontset = XCreateFontSet(dpy, FONTSTR, &missing, &n, &def);
	if (missing) {
		while (n--) {
			warn("missing fontset: %s", missing[n]);
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
		dc.font.xfontstruct = XLoadQueryFont(dpy, FONTSTR);
		if (!dc.font.xfontstruct) {
			die("cannot load font '%s'", FONTSTR);
		}
		dc.font.ascent = dc.font.xfontstruct->ascent;
		dc.font.descent = dc.font.xfontstruct->descent;
	}
	dc.font.height = dc.font.ascent + dc.font.descent;
}

void
setupwsd(void)
{
	/* WSD data */
	wsd.rad = WSDRADIUS;
	wsd.rows = 2*wsd.rad+1;
	wsd.cols = 2*wsd.rad+1;
	wsd.active = false;

	/* WSD window informations */
	wsd.wa.override_redirect = true;
	wsd.wa.background_pixmap = ParentRelative;
	wsd.wa.event_mask = ExposureMask;

	/* dummy workspace for target box */
	wsd.target = initws(0, 0);
	wsd.target->name[0] = 0;

	/* bar input */
	wsd.im = XOpenIM(dpy, NULL, NULL, NULL);
	if (!wsd.im) {
		die("could not open input method");
	}
	wsd.ic = XCreateIC(wsd.im, XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
			XNClientWindow, root, NULL);
	if (!wsd.ic) {
		die("could not open input context");
	}
	XSetICFocus(wsd.ic);
}

void
setws(int x, int y)
{
	Workspace *next=NULL;

	if (locatews(&next, NULL, x, y, NULL) && collision(next)) {
		return;
	}

	/* current workspace */
	if (selmon->selws->nc) {
		hidews(selmon->selws);
	} else {
		detachws(selmon->selws);
		termws(selmon->selws);
	}

	/* next workspace */
	if (next) {
		selmon->selws = next;
	} else {
		selmon->selws = initws(x, y);
		attachws(selmon->selws);
	}

	arrange(selmon);
	updatebar(selmon);
	updatefocus();
}

void
setnmaster(Arg const *arg)
{
	if (!selmon->selws->nmaster && arg->i < 0) {
		return;
	}
	selmon->selws->nmaster = selmon->selws->nmaster+arg->i;
	arrange(selmon);
}

void
shift(Arg const *arg)
{
	unsigned int pos;

	if (selmon->selws->ns < 2) {
		return;
	}
	if (!locateclient(NULL, NULL, &pos, selmon->selws->selcli->win)) {
		warn("attempt to shift non-existent window %d",
				selmon->selws->selcli->win);
	}
	selmon->selws->clients[pos] = selmon->selws->clients[
			(pos+selmon->selws->nc+arg->i)%selmon->selws->nc];
	selmon->selws->clients[(pos+selmon->selws->nc+arg->i)%selmon->selws->nc] =
			selmon->selws->selcli;

	arrange(selmon);
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR) {
		die("could not install SIGCHLD handler");
	}
	/* pid -1 makes it equivalent to wait() (wait for all children);
	 * here we just add WNOHANG */
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(Arg const *arg)
{
	pid_t pid = fork();
	if (pid == 0) {
		execvp(((char const **)arg->v)[0], (char **)arg->v);
		warn("execvp(%s) failed", ((char const **)arg->v)[0]);
		_exit(EXIT_FAILURE);
	} else if (pid < 0) {
		warn("vfork() failed with code %d", pid);
	}
}

void
stdlog(FILE *f, char const *format, ...)
{
	va_list args;
	time_t rawtime;
	struct tm *date;

	/* timestamp */
	time(&rawtime);
	date = localtime(&rawtime);
	fprintf(f, "[%02d:%02d:%02d][%s] ",
			date->tm_hour, date->tm_min, date->tm_sec, APPNAME);

	/* message */
	va_start(args, format);
	vfprintf(f, format, args);
	va_end(args);

	fprintf(f, "\n");
}

void
stepfocus(Arg const *arg)
{
	unsigned int pos;

	if (!selmon->selws->nc) {
		return;
	}
	if (!locateclient(NULL, NULL, &pos, selmon->selws->selcli->win)) {
		warn("attempt to step from non-existing client");
		return;
	}
	push(selmon->selws, selmon->selws->clients[
			(pos+selmon->selws->nc+arg->i)%selmon->selws->nc]);
	updatefocus();
	XRaiseWindow(dpy, selmon->selws->selcli->win);
}

void
stepmon(Arg const *arg)
{
	Monitor *mon;
	unsigned int pos;

	if (!locatemon(&mon, &pos, selmon->selws)) {
		warn("attempt to step from non-existing monitor");
		return;
	}
	selmon = monitors[(pos+nmon+1)%nmon];
	updatefocus();
}

void
stepws(Arg const *arg)
{
	int x, y;
	reltoxy(&x, &y, selmon->selws, arg->i);
	setws(x, y);
}

void
stepwsdbox(Arg const *arg)
{
	Workspace *ws;
	reltoxy(&wsd.target->x, &wsd.target->y, wsd.target, arg->i);
	if (locatews(&ws, NULL, wsd.target->x, wsd.target->y, NULL)) {
		strcpy(wsd.target->name, ws->name);
	} else {
		wsd.target->name[0] = 0;
	}
	updatewsdbar(NULL, wsd.target->name);
}

void
termclient(Client *c)
{
	free(c);
}

void
termmon(Monitor *mon)
{
	XDestroyWindow(dpy, mon->bar.win);
	free(mon);
}

void
termws(Workspace *ws)
{
	if (ws->nc) {
		warn("destroying non-empty workspace");
	}
	free(ws);
}

unsigned int
textwidth(char const *str, unsigned int len)
{
	XRectangle r;
	if (dc.font.xfontset) {
		XmbTextExtents(dc.font.xfontset, str, len, NULL, &r);
		return r.width;
	}
	return XTextWidth(dc.font.xfontstruct, str, len);
}

/* TODO move to layouts.h */
void
tile(Monitor *mon)
{
	Client **tiled;
	unsigned int ncm, i, ntiled, w, h;
	int x;

	if (!mon->selws->nc) {
		return;
	}

	/* get non-floating clients (TODO move to separate function) */
	tiled = calloc(mon->selws->nc, sizeof(Client *));
	if (!tiled) {
		die("could not allocate %u bytes for client list", mon->selws->nc);
	}
	for (ntiled = 0, i = 0; i < mon->selws->nc; i++) {
		if (!mon->selws->clients[i]->floating) {
			tiled[ntiled++] = mon->selws->clients[i];
		}
	}
	tiled = realloc(tiled, ntiled*sizeof(Client *));
	if (!tiled && ntiled) {
		die("could not allocate %u bytes for client list",
				ntiled*sizeof(Client *));
	}

	/* draw master area */
	ncm = MIN(mon->selws->nmaster, ntiled);
	if (ncm) {
		x = mon->wx;
		w = mon->selws->nmaster >= ntiled ? mon->ww : mon->selws->mfact*mon->ww;
		h = mon->wh/ncm;
		for (i = 0; i < ncm; i++) {
			moveresize(tiled[i], x, mon->wy + i*h,
					w - 2*BORDERWIDTH, h - 2*BORDERWIDTH);
		}
	}
	if (ncm == ntiled) {
		return;
	}

	/* draw stack area */
	x = mon->wx+(ncm ? mon->selws->mfact*mon->ww : 0);
	w = ncm ? mon->ww-x : mon->ww;
	h = mon->wh/(ntiled-ncm);
	for (i = ncm; i < ntiled; i++) {
		moveresize(tiled[i], x, mon->wy + (i-ncm)*h,
				w - 2*BORDERWIDTH, h - 2*BORDERWIDTH);
	}
}

void
togglefloat(Arg const *arg)
{
	Client *c;

	if (!selmon->selws->nc) {
		return;
	}
	c = selmon->selws->selcli;
	if ((c->floating = !c->floating)) {
		XRaiseWindow(dpy, c->win);
	} else {
		XLowerWindow(dpy, c->win);
	}
	arrange(selmon);
	updatefocus();
}

void
togglewsd(Arg const *arg)
{
	unsigned int i;

	wsd.active = !wsd.active;

	if (!wsd.active) {
		XUngrabKeyboard(dpy, selmon->selws->wsdbox);
		XLowerWindow(dpy, selmon->bar.win);
		for (i = 0; i < nws; i++) {
			if (workspaces[i]->wsdbox) {
				XDestroyWindow(dpy, workspaces[i]->wsdbox);
				workspaces[i]->wsdbox = 0;
			}
		}
		XDestroyWindow(dpy, wsd.target->wsdbox);
		wsd.target->wsdbox = 0;
		updatebar(selmon);
		grabkeys();
		return;
	}

	/* create a box for each workspace and hide it */
	wsd.w = selmon->ww/wsd.cols;
	wsd.h = selmon->h/wsd.rows;
	for (i = 0; i < nws; i++) {
		workspaces[i]->wsdbox = initwsdbox();
	}
	wsd.target->wsdbox = initwsdbox();

	/* initial target is the current workspace (make a "0-step") */
	wsd.target->x = selmon->selws->x;
	wsd.target->y = selmon->selws->y;
	stepwsdbox(&((Arg const) { .i = NO_DIRECTION })); /* updates input bar */

	/* grab keyboard, raise bar, we're in WSD mode now! */
	XGrabKeyboard(dpy, selmon->selws->wsdbox, false, GrabModeAsync,
			GrabModeAsync, CurrentTime);
	XRaiseWindow(dpy, selmon->bar.win);

	/* initial update */
	selmon->bar.buffer[0] = 0;
	selmon->bar.cursor = 0;
	updatebar(selmon);
	updatewsdmap();
}

size_t
unifyscreens(XineramaScreenInfo **list, size_t len)
{
	unsigned int i, j, n;
	bool dup;

	/* reserve enough space */
	XineramaScreenInfo *unique = calloc(len, sizeof(XineramaScreenInfo));
	if (!unique && len) {
		die("could not allocate %d bytes for screen info list",
				len*sizeof(XineramaScreenInfo));
	}

	for (i = 0, n = 0; i < len; i++) {
		dup = false;
		for (j = 0; j < i; j++) {
			if ((*list)[j].x_org == (*list)[i].x_org
					&& (*list)[j].y_org == (*list)[i].y_org
					&& (*list)[j].width == (*list)[i].width
					&& (*list)[j].height == (*list)[i].height) {
				dup = true;
				break;
			}
		}
		if (!dup) {
			unique[n].x_org = (*list)[i].x_org;
			unique[n].y_org = (*list)[i].y_org;
			unique[n].width = (*list)[i].width;
			unique[n].height = (*list)[i].height;
			n++;
		}
	}
	XFree(*list);
	*list = unique;
	*list = realloc(*list, n*sizeof(XineramaScreenInfo)); /* fix size */
	return n;
}

void
unmapnotify(XEvent *e)
{
	debug("unmapnotify(%d)", e->xunmap.window);

	Client *c;
	Workspace *ws;
	Monitor *mon;

	if (locateclient(&ws, &c, NULL, e->xunmap.window)) {
		detachclient(c);
		termclient(c);
		if (locatemon(&mon, NULL, ws)) {
			arrange(mon);
			updatefocus();
		}
	}
}

void
updatebar(Monitor *mon)
{
	if (mon->bx != mon->x || mon->by != mon->y || mon->bw != mon->w) {
		mon->bx = mon->x;
		mon->by = mon->y;
		mon->bw = mon->w;
		XMoveResizeWindow(dpy, mon->bar.win, mon->bx, mon->by, mon->bw,mon->bh);
	}
	if (!wsd.active) {
		strcpy(mon->bar.buffer, mon->selws->name);
	}
	renderbar(mon);
}

void
updatefocus(void)
{
	unsigned int i, j;
	bool sel;
	Monitor *mon;

	for (i = 0; i < nmon; i++) {
		mon = monitors[i];
		sel = mon == selmon;

		/* empty monitor: don't focus anything */
		if (!mon->selws->ns) {
			XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
			continue;
		}

		/* update selected client */
		mon->selws->selcli = mon->selws->stack[mon->selws->ns-1];

		/* draw borders and restack */
		for (j = 0; j < (sel ? mon->selws->ns-1 : mon->selws->ns); j++) {
			XSetWindowBorder(dpy, mon->selws->stack[j]->win, CBORDERNORM);
			grabbuttons(mon->selws->stack[j], false);
		}
		if (sel) {
			XSetWindowBorder(dpy, mon->selws->selcli->win, CBORDERSEL);
			XSetInputFocus(dpy, mon->selws->selcli->win, RevertToPointerRoot,
					CurrentTime);
			grabbuttons(mon->selws->stack[j], true);
		}
	}
}

void
updategeom(void)
{
	int i, n;
	XineramaScreenInfo *info;
	Monitor *mon;

	if (!XineramaIsActive(dpy)) {
		while (nmon > 1) {
			mon = monitors[1];
			detachmon(mon);
			termmon(mon);
		}
		if (!nmon) {
			selmon = initmon();
			attachmon(selmon);
		}
		updatemon(selmon, 0, 0, DisplayWidth(dpy, screen),
				DisplayHeight(dpy, screen));
		return;
	}

	info = XineramaQueryScreens(dpy, &n);
	n = unifyscreens(&info, n);

	if (n < nmon) { /* screen detached */
		for (i = nmon-1; i >= n; i--) {
			mon = monitors[i];
			hidews(mon->selws);
			detachmon(mon);
			termmon(mon);
		}
	}
	for (i = 0; i < n; i++) {
		if (i >= nmon) { /* screen attached */
			mon = initmon();
			attachmon(mon);
		}
		selmon = monitors[i];
		updatemon(selmon, info[i].x_org, info[i].y_org, info[i].width, info[i].height);
	}
	free(info);
}

void
updatemon(Monitor *mon, int x, int y, unsigned int w, unsigned int h)
{
	mon->x = x;
	mon->y = y;
	mon->w = w;
	mon->h = h;
	updatebar(mon);

	mon->wx = mon->x;
	mon->wy = mon->y+mon->bh;
	mon->ww = mon->w;
	mon->wh = mon->h-mon->bh;
	arrange(mon);
}

void
updatesizehints(Client *c)
{
	long size;
	XSizeHints hints;

	if (!XGetWMNormalHints(dpy, c->win, &hints, &size)) {
		warn("XGetWMNormalHints() failed");
		return;
	}
	if (hints.flags & PBaseSize) {
		c->basew = hints.base_width;
		c->baseh = hints.base_height;
	} else {
		c->basew = c->baseh = 0;
	}
	if (hints.flags & PResizeInc) {
		c->incw = hints.width_inc;
		c->inch = hints.height_inc;
	} else {
		c->incw = c->inch = 0;
	}
}

void
updatewsdmap(void)
{
	unsigned int i;

	if (!wsd.active) {
		return;
	}

	for (i = 0; i < nws; i++) {
		updatewsdbox(workspaces[i]);
	}
	updatewsdbox(wsd.target);
	XSync(dpy, false);
}

void
updatewsdbar(XEvent *e, char const *name)
{
	Status status;
	char code[20], ansi;
	unsigned int i, n;
	KeySym keysym;
	Workspace *ws;
	XKeyPressedEvent *ev = &e->xkey;

	/* if name is set, replace current buffer by name */
	if (name) {
		strncpy(selmon->bar.buffer, name, 256);
		selmon->bar.cursor = strlen(selmon->bar.buffer);
		updatebar(selmon);
		return;
	}

	/* if IM is filtering the current input (e.g. dead key), ignore it */
	if (XFilterEvent(e, selmon->bar.win)) {
		return;
	}

	/* catch special keys */
	keysym = XLookupKeysym(ev, 0);
	ansi = keytoansi(&keysym);
	if (!ansi) {
		n = Xutf8LookupString(wsd.ic, ev, code, 20, NULL, &status);
		if (!n) {
			return;
		} else if (n == 1 && (code[0] < 0x20 || code[0] == 0x7F)) {
			ansi = code[0];
		} else {
			if (strlen(selmon->bar.buffer)+n < 256) {
				for (i = strlen(selmon->bar.buffer)+n;
						i >= selmon->bar.cursor+n; i--) {
					selmon->bar.buffer[i] = selmon->bar.buffer[i-n];
				}
				strncpy(selmon->bar.buffer+selmon->bar.cursor, code, n);
				selmon->bar.cursor += n;
				updatebar(selmon);
			} else {
				warn("WSD bar: buffer is full");
			}
			return;
		}
	}

	/* ANSI escape codes */
	switch (ansi) {
		case 0x01: /* ^A | Home */
			selmon->bar.cursor = 0;
			break;
		case 0x02: /* ^B | Left */
			selmon->bar.cursor = MAX(selmon->bar.cursor-1, 0);
			break;
		case 0x03: /* ^C |        */
		case 0x1B: /*      Escape */
			togglewsd(NULL);
			break;
		case 0x04: /* ^D | Delete */
		case 0x7F: /*    | Delete (soft) */
			for (i = selmon->bar.cursor; i < strlen(selmon->bar.buffer); i++) {
				selmon->bar.buffer[i] = selmon->bar.buffer[i+1];
			}
			break;
		case 0x05: /* ^E | End */
			selmon->bar.cursor = strlen(selmon->bar.buffer);
			break;
		case 0x06: /* ^F | Right */
			selmon->bar.cursor = MIN(selmon->bar.cursor+1, strlen(selmon->bar.buffer));
			break;
		case 0x08: /* ^H | Backspace */
			if (selmon->bar.cursor) {
				for (i = --selmon->bar.cursor; i < strlen(selmon->bar.buffer); i++) {
					selmon->bar.buffer[i] = selmon->bar.buffer[i+1];
				}
			}
			break;
		case 0x0A: /* ^J          */
		case 0x0D: /* ^M | Return */
			if (e->xkey.state&ControlMask && keysym != XK_j) {
				if (strlen(selmon->bar.buffer) && locatews(&ws, NULL,
						wsd.target->x, wsd.target->y, NULL)) {
					strncpy(ws->name, selmon->bar.buffer, 255);
					ws->name[255] = 0;
				}
			} else {
				if (strlen(selmon->bar.buffer) && locatews(&ws, NULL,
						0, 0, selmon->bar.buffer)) {
					setws(ws->x, ws->y);
				} else {
					setws(wsd.target->x, wsd.target->y);
				}
			}
			togglewsd(NULL);
			break;
		case 0x0B: /* ^K */
			selmon->bar.buffer[selmon->bar.cursor] = 0;
			break;
		case 0x0E: /* ^N | Down */
			/* TODO */
			break;
		case 0x10: /* ^P | Up */
			/* TODO */
			break;
		case 0x15: /* ^U */
			strcpy(selmon->bar.buffer, selmon->bar.buffer+selmon->bar.cursor);
			selmon->bar.cursor = 0;
			break;
		default:
			warn("unknown keycode: %X", code[0]);
	}
	updatebar(selmon);
}

void
updatewsdbox(Workspace *ws)
{
	int c, r, x, y;

	/* hide box if not in range */
	if (ws->x < wsd.target->x-wsd.rad || ws->x > wsd.target->x+wsd.rad ||
			ws->y < wsd.target->y-wsd.rad || ws->y > wsd.target->y+wsd.rad) {
		XMoveWindow(dpy, ws->wsdbox, -wsd.w, -wsd.h);
		return;
	}

	/* show box */
	c = ws->x + wsd.rad - wsd.target->x;
	r = ws->y + wsd.rad - wsd.target->y;
	x = selmon->ww/2 - wsd.w*wsd.rad - wsd.w/2 + c*wsd.w + selmon->wx;
	y = selmon->wh/2 - wsd.h*wsd.rad - wsd.h/2 + r*wsd.h + selmon->wy;
	XMoveWindow(dpy, ws->wsdbox, x, y);
	XRaiseWindow(dpy, ws->wsdbox);

	renderwsdbox(ws);
}

int
xerror(Display *dpy, XErrorEvent *ee)
{
	char es[256];

	/* only display error on this error instead of crashing */
	if (ee->error_code == BadWindow) {
		XGetErrorText(dpy, ee->error_code, es, 256);
		warn("%s (ID %d) after request %d", es, ee->error_code, ee->error_code);
		return 0;
	}

	/* call default error handler (might call exit) */
	return xerrorxlib(dpy, ee);
}

void
zoom(Arg const *arg)
{
	unsigned int i, pos;
	Client *c;

	if (!selmon->selws->nc) {
		return;
	}

	if (!locateclient(NULL, &c, &pos, selmon->selws->selcli->win)) {
		warn("attempt to zoom non-existing window %d",
				selmon->selws->selcli->win);
		return;
	}

	if (!pos) {
		/* window is at the top */
		if (selmon->selws->nc > 1) {
			selmon->selws->clients[0] = selmon->selws->clients[1];
			selmon->selws->clients[1] = c;
			push(selmon->selws, selmon->selws->clients[0]);
			updatefocus();
		} else {
			return;
		}
	} else {
		/* window is somewhere else */
		for (i = pos; i > 0; i--) {
			selmon->selws->clients[i] = selmon->selws->clients[i-1];
		}
		selmon->selws->clients[0] = c;
	}
	arrange(selmon);
}

int
main(int argc, char **argv)
{
	if (!(dpy = XOpenDisplay(NULL))) {
		die("could not open X");
	}
	note("starting ...");
	setup();
	scan();
	custom_startup();
	run();
	custom_shutdown();
	cleanup();
	XCloseDisplay(dpy);
	note("shutting down ...");
	if (restarting) {
		execl(APPNAME, APPNAME, NULL); /* TODO change to execlp */
	}
	return EXIT_SUCCESS;
}

