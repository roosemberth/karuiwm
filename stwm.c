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
#include <X11/keysym.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xinerama.h>

/* macros */
#define APPNAME "stwm"
#define CLIENTMASK EnterWindowMask
#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

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
	int x, y;
	unsigned int w, h;
	char name[256];
	Window win;
	bool floating;
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
	unsigned int sx, sy, sw, sh; /* screen dimensions */
	unsigned int bx, by, bw, bh; /* status bar dimensions */
	unsigned int wx, wy, ww, wh; /* workspace dimensions */
	Window barwin;               /* status bar window */
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
	int w, h;
	int rows, cols, rad;
	bool shown;
	Window barwin;
	char barbuf[256];
	unsigned int barcur;
	XIM im;
	XIC ic;
} WorkspaceDialog;

/* functions */
static void arrange(void);
static void attach(Workspace *, Client *);
static void attachws(Workspace *);
static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void cleanup(void);
static void clientmessage(XEvent *);
static void configurerequest(XEvent *);
static void configurenotify(XEvent *);
static void createnotify(XEvent *);
static void destroynotify(XEvent *);
static void detach(Client *);
static void detachmon(Monitor *);
static void detachws(Workspace *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void grabbuttons(void);
static void grabkeys(void);
static void hide(Client *);
static void hidews(Workspace *);
static Client *initclient(Window, bool);
static Monitor *initmon(void);
static void updatemons(void);
static Workspace *initws(int, int);
static Window initwsdbox(void);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void killclient(Arg const *);
static bool locate(Workspace **, Client **, unsigned int *, Window const);
static bool locatemon(Monitor **, unsigned int *, Workspace const *);
static bool locatews(Workspace **, unsigned int *, int, int, char const *);
static void mappingnotify(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void move(Arg const *);
static void movews(Arg const *);
static void place(Client *, int, int, unsigned int, unsigned int);
static void pop(Workspace *, Client *);
static void propertynotify(XEvent *);
static void push(Workspace *, Client *);
static void quit(Arg const *);
static void renamews(Workspace *, char const *);
static void renderbar(void);
static void renderwsdbar(void);
static void renderwsdbox(Workspace *);
static void restart(Arg const *);
static void run(void);
static void scan(void);
static void setmfact(Arg const *);
static void setup(void);
static void setupbar(void);
static void setupfont(void);
static void setupwsd(void);
static void setnmaster(Arg const *);
static void setws(int, int);
static void shift(Arg const *);
static void sigchld(int);
static void spawn(Arg const *);
static void stdlog(FILE *, char const *, ...);
static void stepfocus(Arg const *);
static void stepws(Arg const *);
static void stepwsdbox(Arg const *arg);
static unsigned int textwidth(char const *, unsigned int);
static void tile(void);
static void togglewsd(Arg const *);
static size_t unifyscreens(XineramaScreenInfo **, size_t);
static void unmapnotify(XEvent *);
static void updatebar(void);
static void updatefocus(void);
static void updatewsdmap(void);
static void updatewsdbar(XEvent *, char const *);
static void updatewsdbox(Workspace *);
static int xerror(Display *, XErrorEvent *);
static int (*xerrorxlib)(Display *, XErrorEvent *);
static void zoom(Arg const *);

/* event handlers, as array to allow O(1) access; codes in X.h */
static void (*handle[LASTEvent])(XEvent *) = {
	[ButtonPress]      = buttonpress,      /* 4*/
	[ButtonRelease]    = buttonrelease,    /* 5*/
	[ClientMessage]    = clientmessage,    /*33*/
	[ConfigureNotify]  = configurenotify,  /*22*/
	[ConfigureRequest] = configurerequest, /*23*/
	[CreateNotify]     = createnotify,     /*16*/
	[DestroyNotify]    = destroynotify,    /*17*/
	[EnterNotify]      = enternotify,      /* 7*/
	[Expose]           = expose,           /*12*/
	[KeyPress]         = keypress,         /* 2*/
	[KeyRelease]       = keyrelease,       /* 3*/
	[MapRequest]       = maprequest,       /*20*/
	[MappingNotify]    = mappingnotify,    /*34*/
	[MotionNotify]     = motionnotify,     /* 6*/
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
arrange(void)
{
	unsigned int i;
	for (i = 0; i < selmon->selws->nc; i++) {
		XSelectInput(dpy, selmon->selws->clients[i]->win, 0);
	}
	/* TODO use array of layouts */
	tile();
	for (i = 0; i < selmon->selws->nc; i++) {
		XSelectInput(dpy, selmon->selws->clients[i]->win, CLIENTMASK);
	}
}

void
attach(Workspace *ws, Client *c)
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
	debug("buttonpress(%d)", e->xbutton.window);
	/* TODO */
}

void
buttonrelease(XEvent *e)
{
	debug("buttonrelease(%d)", e->xbutton.window);
	/* TODO */
}

void
cleanup(void)
{
	unsigned int i, j;

	/* workspaces and their clients */
	for (i = 0; i < nws; i++) {
		for (j = 0; j < workspaces[i]->nc; j++) {
			free(workspaces[i]->clients[j]);
		}
		free(workspaces[i]->clients);
		free(workspaces[i]);
	}
	free(workspaces);

	/* monitors */
	for (i = 0; i < nmon; i++) {
		free(monitors[i]);
	}
	free(monitors);

	/* special windows */
	XDestroyWindow(dpy, selmon->barwin);
	XDestroyWindow(dpy, wsd.barwin);
	XDestroyWindow(dpy, wsd.target->wsdbox);
	free(wsd.target);

	/* graphic context */
	XFreeGC(dpy, dc.gc);
}

void
clientmessage(XEvent *e)
{
	debug("clientmessage(%d)", e->xclient.window);
	/* TODO */
}

void
configurenotify(XEvent *e)
{
	debug("configurenotify(%d)", e->xconfigure.window);
	/* TODO root window notifications */
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
	if (!FORCESIZE || !locate(NULL, &c, NULL, ev->window)) {
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
createnotify(XEvent *e)
{
	debug("createnotify(%d)", e->xcreatewindow.window);
	/* TODO */
}

void
destroynotify(XEvent *e)
{
	debug("destroynotify(%d)", e->xdestroywindow.window);
	/* TODO */
}

void
detach(Client *c)
{
	unsigned int i;
	Workspace *ws;

	if (!locate(&ws, &c, &i, c->win)) {
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

	/* remove workspace if not selected and last client was removed */
	if (ws != selmon->selws && !ws->nc) {
		detachws(ws);
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
		updatewsdmap();
	}
}

void
enternotify(XEvent *e)
{
	debug("enternotify(%d)", e->xcrossing.window);

	unsigned int pos;
	Workspace *ws;
	Client *c;

	if (!locate(&ws, &c, &pos, e->xcrossing.window) || ws != selmon->selws) {
		warn("attempt to enter unhandled/invisible window %d",
				e->xcrossing.window);
		return;
	}

	push(selmon->selws, c);
	updatefocus();
}

void
expose(XEvent *e)
{
	debug("expose(%d)", e->xexpose.window);

	unsigned int i;

	if (e->xexpose.window == selmon->barwin) {
		renderbar();
	}
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
grabbuttons(void)
{
	/* TODO */
}

void
grabkeys(void)
{
	int i;

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

	/* common actions (maprequest() and scan()) */
	attach(selmon->selws, c);
	arrange();
	XSetWindowBorderWidth(dpy, c->win, BORDERWIDTH);
	XSelectInput(dpy, c->win, CLIENTMASK);
	updatebar();
	return c;
}

Monitor *
initmon(void)
{
	unsigned int x;
	Monitor *mon;
	Workspace *ws;

	mon = malloc(sizeof(Monitor));
	if (!mon) {
		die("could not allocate %u bytes for monitor", sizeof(Monitor));
	}

	/* assign workspace */
	for (x = 0;; x++) {
		if (!locatews(&ws, NULL, x, 0, NULL)) {
			mon->selws = initws(x, 0);
			attachws(mon->selws);
			break;
		} else if (!locatemon(NULL, NULL, ws)) {
			mon->selws = ws;
			break;
		}
	}
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
	return XCreateWindow(dpy, root, -wsd.w, -wsd.h,
			wsd.w-2*WSDBORDERWIDTH, wsd.h-2*WSDBORDERWIDTH, 0,
			DefaultDepth(dpy, screen), CopyFromParent,
			DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wsd.wa);
}
void
keypress(XEvent *e)
{
	debug("keypress()");

	unsigned int i;
	KeySym keysym = XLookupKeysym(&e->xkey, 0);

	/* catch normal keys */
	if (!wsd.shown) {
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
	debug("keyrelease()");
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
locate(Workspace **ws, Client **c, unsigned int *pos, Window w)
{
	unsigned int i, j;

	/* search current workspace */
	for (i = 0; i < selmon->selws->nc; i++) {
		if (selmon->selws->clients[i]->win == w) {
			if (ws) *ws = selmon->selws;
			if (c) *c = selmon->selws->clients[i];
			if (pos) *pos = i;
			return true;
		}
	}
	/* search all the other workspaces */
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
		XMapWindow(dpy, c->win);
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
motionnotify(XEvent *e)
{
	debug("motionnotify(%d)", e->xmotion.window);
	/* TODO */
}

void
move(Arg const *arg)
{
	Client *c = selmon->selws->selcli;
	detach(c);
	stepws(arg);
	attach(selmon->selws, c);
	arrange();
	updatefocus();
	updatebar();
}

void
movews(Arg const *arg)
{
	Workspace *src=NULL, *dst=NULL;
	int x=wsd.target->x, y=wsd.target->y;

	switch (arg->i) {
		case LEFT:  x--; break;
		case RIGHT: x++; break;
		case UP:    y--; break;
		case DOWN:  y++; break;
	}

	locatews(&src, NULL, wsd.target->x, wsd.target->y, NULL);
	locatews(&dst, NULL, x, y, NULL);
	if (src) {
		src->x = x;
		src->y = y;
	}
	if (dst) {
		dst->x = wsd.target->x;
		dst->y = wsd.target->y;
	}
	wsd.target->x = x;
	wsd.target->y = y;
}

void
place(Client *c, int x, int y, unsigned int w, unsigned int h)
{
	XMoveResizeWindow(dpy, c->win, c->x=x, c->y=y, c->w=w, c->h=h);
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
renderbar(void)
{
	XSetForeground(dpy, dc.gc, CBORDERNORM);
	XFillRectangle(dpy, selmon->barwin, dc.gc, 0, 0, selmon->bw, selmon->bh);
	XSetForeground(dpy, dc.gc, 0x888888);
	XDrawString(dpy, selmon->barwin, dc.gc, 0, dc.font.ascent+1,
			selmon->selws->name, strlen(selmon->selws->name));
	XSync(dpy, false);
}

void
renderwsdbar(void)
{
	int cursorx;

	/* text */
	XSetForeground(dpy, dc.gc, CBGSEL);
	XFillRectangle(dpy, wsd.barwin, dc.gc, 0, 0, selmon->bw, selmon->bh);
	XSetForeground(dpy, dc.gc, CSEL);
	Xutf8DrawString(dpy, wsd.barwin, dc.font.xfontset, dc.gc, 6,
			dc.font.ascent+1, wsd.barbuf, strlen(wsd.barbuf));

	/* cursor */
	cursorx = textwidth(wsd.barbuf, wsd.barcur)+5;
	XFillRectangle(dpy, wsd.barwin, dc.gc, cursorx, 1, 1, dc.font.height);
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
	note("found %d clients from last session", nwins);
	for (i = 0; i < nwins; i++) {
		c = initclient(wins[i], true);
		if (c) {
			updatefocus();
		}
	}
}

void
setmfact(Arg const *arg)
{
	selmon->selws->mfact = MAX(0.1, MIN(0.9, selmon->selws->mfact+arg->f));
	arrange();
}

void
setup(void)
{
	XSetWindowAttributes wa;

	/* kill zombies */
	sigchld(0);

	/* locale */
	if (!setlocale(LC_ALL, "") || !XSupportsLocale()) {
		die("could not set locale");
	}

	/* monitors */
	debug("1");
	screen = DefaultScreen(dpy);
	debug("2");
	root = RootWindow(dpy, screen);
	debug("3");
	updatemons();
	debug("4");
	selmon->sw = DisplayWidth(dpy, screen);
	debug("5");
	selmon->sh = DisplayHeight(dpy, screen);
	debug("6");
	xerrorxlib = XSetErrorHandler(xerror);
	debug("7");
	dc.gc = XCreateGC(dpy, root, 0, NULL);

	/* input (cursor/keys) */
	debug("8");
	cursor[CURSOR_NORMAL] = XCreateFontCursor(dpy, XC_left_ptr);
	debug("9");
	cursor[CURSOR_RESIZE] = XCreateFontCursor(dpy, XC_sizing);
	debug("10");
	cursor[CURSOR_MOVE] = XCreateFontCursor(dpy, XC_fleur);
	debug("11");
	wa.cursor = cursor[CURSOR_NORMAL];
	debug("12");
	XChangeWindowAttributes(dpy, root, CWCursor, &wa);
	debug("13");
	grabbuttons();
	debug("14");
	grabkeys();

	/* event mask */
	debug("15");
	wa.event_mask = SubstructureNotifyMask|SubstructureRedirectMask|
			KeyPressMask;
	debug("16");
	XChangeWindowAttributes(dpy, root, CWEventMask, &wa);

	/* font */
	debug("17");
	setupfont();

	/* status bar */
	debug("18");
	setupbar();

	/* workspace dialog */
	debug("19");
	setupwsd();
}

void
setupbar(void)
{
	XSetWindowAttributes wa;

	selmon->bx = 0;
	selmon->by = 0; /* TODO allow bar to be at bottom */
	selmon->bh = dc.font.height + 2;
	selmon->bw = selmon->sw;
	selmon->wx = 0;
	selmon->wy = selmon->bh;
	selmon->ww = selmon->sw;
	selmon->wh = selmon->sh-selmon->bh;

	wa.override_redirect = true;
	wa.background_pixmap = ParentRelative;
	wa.event_mask = ExposureMask;
	selmon->barwin = XCreateWindow(dpy, root, selmon->bx, selmon->by, selmon->bw,
			selmon->bh, 0, DefaultDepth(dpy, screen), CopyFromParent,
			DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	XMapRaised(dpy, selmon->barwin);
	updatebar();
}

void
setupfont(void)
{
	XFontStruct **xfonts;
	char **xfontnames;
	char *def, **missing;
	int n;

	debug("17.1");
	dc.font.xfontset = XCreateFontSet(dpy, FONTSTR, &missing, &n, &def);
	debug("17.2");
	if (missing) {
		debug("17.3");
		while (n--) {
			debug("17.4");
			warn("missing fontset: %s", missing[n]);
		}
		debug("17.5");
		XFreeStringList(missing);
		debug("17.6");
	}
	debug("17.7");

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
	wsd.w = selmon->ww/wsd.cols;
	wsd.h = selmon->wh/wsd.rows;
	wsd.shown = false;

	/* WSD window informations */
	wsd.wa.override_redirect = true;
	wsd.wa.background_pixmap = ParentRelative;
	wsd.wa.event_mask = ExposureMask;

	/* target box */
	wsd.target = initws(0, 0);
	wsd.target->name[0] = 0;
	wsd.target->wsdbox = initwsdbox();
	XMapRaised(dpy, wsd.target->wsdbox);

	/* bar */
	wsd.barwin = XCreateWindow(dpy, root, -selmon->bw, -selmon->bh, selmon->bw,
			selmon->bh, 0, DefaultDepth(dpy, screen), CopyFromParent,
			DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wsd.wa);
	XMapWindow(dpy, wsd.barwin);

	/* bar input */
	wsd.im = XOpenIM(dpy, NULL, NULL, NULL);
	if (!wsd.im) {
		die("could not open input method");
	}
	wsd.ic = XCreateIC(wsd.im, XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
			XNClientWindow, wsd.barwin, NULL);
	if (!wsd.ic) {
		die("could not open input context");
	}
	XSetICFocus(wsd.ic);
}

void
setws(int x, int y)
{
	Workspace *next;

	/* current workspace */
	if (selmon->selws->nc) {
		hidews(selmon->selws);
	} else {
		detachws(selmon->selws);
		free(selmon->selws);
	}

	/* next workspace */
	if (locatews(&next, NULL, x, y, NULL)) {
		selmon->selws = next;
	} else {
		selmon->selws = initws(x, y);
		attachws(selmon->selws);
	}
	arrange();
	updatebar();
	updatefocus();
}

void
setnmaster(Arg const *arg)
{
	if (!selmon->selws->nmaster && arg->i < 0) {
		return;
	}
	selmon->selws->nmaster = selmon->selws->nmaster+arg->i;
	arrange();
}

void
shift(Arg const *arg)
{
	unsigned int pos;

	if (selmon->selws->ns < 2) {
		return;
	}
	if (!locate(NULL, NULL, &pos, selmon->selws->selcli->win)) {
		warn("attempt to shift non-existent window %d",
				selmon->selws->selcli->win);
	}
	selmon->selws->clients[pos] = selmon->selws->clients[
			(pos+selmon->selws->nc+arg->i)%selmon->selws->nc];
	selmon->selws->clients[(pos+selmon->selws->nc+arg->i)%selmon->selws->nc] =
			selmon->selws->selcli;

	arrange();
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
	locate(NULL, NULL, &pos, selmon->selws->selcli->win);
	push(selmon->selws, selmon->selws->clients[
			(pos+selmon->selws->nc+arg->i)%selmon->selws->nc]);
	updatefocus();
}

void
stepwsdbox(Arg const *arg)
{
	Workspace *ws;

	switch (arg->i) {
		case LEFT:  wsd.target->x--; break;
		case RIGHT: wsd.target->x++; break;
		case UP:    wsd.target->y--; break;
		case DOWN:  wsd.target->y++; break;
		default: /* NO_DIRECTION */  break;
	}
	if (locatews(&ws, NULL, wsd.target->x, wsd.target->y, NULL)) {
		strcpy(wsd.target->name, ws->name);
	} else {
		wsd.target->name[0] = 0;
	}
	updatewsdbar(NULL, wsd.target->name);
}

void
stepws(Arg const *arg)
{
	int x=selmon->selws->x, y=selmon->selws->y;

	switch (arg->i) {
		case LEFT:  x = selmon->selws->x-1; break;
		case RIGHT: x = selmon->selws->x+1; break;
		case UP:    y = selmon->selws->y-1; break;
		case DOWN:  y = selmon->selws->y+1; break;
		default: /* NO_DIRECTION */ break;
	}
	setws(x, y);
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
tile(void)
{
	unsigned int ncm, i, w, h;
	int x;

	if (!selmon->selws->nc) {
		return;
	}

	/* draw master area */
	ncm = MIN(selmon->selws->nmaster, selmon->selws->nc);
	if (ncm) {
		x = selmon->wx;
		w = selmon->selws->nmaster >= selmon->selws->nc ?
				selmon->ww : selmon->selws->mfact*selmon->ww;
		h = selmon->wh/ncm;
		for (i = 0; i < ncm; i++) {
			place(selmon->selws->clients[i], x, selmon->wy+i*h, w-2*BORDERWIDTH,
					h-2*BORDERWIDTH);
		}
	}
	if (ncm == selmon->selws->nc) {
		return;
	}

	/* draw stack area */
	x = ncm ? selmon->selws->mfact*selmon->ww : 0;
	w = ncm ? selmon->ww-x : selmon->ww;
	h = selmon->wh/(selmon->selws->nc-ncm);
	for (i = ncm; i < selmon->selws->nc; i++) {
		place(selmon->selws->clients[i], x, selmon->wy+(i-ncm)*h,
				w-2*BORDERWIDTH, h-2*BORDERWIDTH);
	}
}

void
togglewsd(Arg const *arg)
{
	unsigned int i;

	/* if WSD is already shown, remove it and return */
	if (wsd.shown) {
		for (i = 0; i < nws; i++) {
			if (workspaces[i]->wsdbox) {
				XDestroyWindow(dpy, workspaces[i]->wsdbox);
				workspaces[i]->wsdbox = 0;
			}
		}
		/* hide target box and input bar */
		XMoveWindow(dpy, wsd.target->wsdbox, -wsd.w, -wsd.h);
		XMoveWindow(dpy, wsd.barwin, -selmon->bh, -selmon->bw);
		wsd.shown = false;
		XUngrabKeyboard(dpy, selmon->selws->wsdbox);
		grabkeys();
		updatefocus();
		return;
	}

	/* create a box for each workspace and hide it */
	for (i = 0; i < nws; i++) {
		workspaces[i]->wsdbox = initwsdbox();
		XMapWindow(dpy, workspaces[i]->wsdbox);
	}

	/* show input bar */
	XMoveWindow(dpy, wsd.barwin, selmon->bx, selmon->by);
	XRaiseWindow(dpy, wsd.barwin);

	/* initial target is the current workspace (make a "0-step") */
	wsd.target->x = selmon->selws->x;
	wsd.target->y = selmon->selws->y;
	stepwsdbox(&((Arg const) { .i = NO_DIRECTION })); /* updates input bar */

	/* grab keyboard, now we're in WSD mode */
	XGrabKeyboard(dpy, selmon->selws->wsdbox, false, GrabModeAsync,
			GrabModeAsync, CurrentTime);
	wsd.shown = true;

	/* initial update for map */
	updatewsdmap();
}

size_t
unifyscreens(XineramaScreenInfo **list, size_t len)
{
	unsigned int i, j, n;
	bool dub;

	/* reserve enough space */
	XineramaScreenInfo *unique = calloc(len, sizeof(XineramaScreenInfo));

	for (i = 0, n = 0; i < len; i++) {
		dub = false;
		for (j = 0; j < i; j++) {
			if (list[j]->x_org == list[i]->x_org
					&& list[j]->y_org == list[i]->y_org
					&& list[j]->width == list[i]->width
					&& list[j]->height == list[i]->height) {
				dub = true;
				break;
			}
		}
		if (!dub) {
			unique[n].x_org = list[i]->x_org;
			unique[n].y_org = list[i]->y_org;
			unique[n].width = list[i]->width;
			unique[n].height = list[i]->height;
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

	if (locate(&ws, &c, NULL, e->xunmap.window)) {
		detach(c);
		free(c);
		if (ws == selmon->selws) {
			arrange();
			updatefocus();
		}
	}
}

void
updatebar(void)
{
	renderbar();
}

void
updatefocus(void)
{
	unsigned int i;

	if (!selmon->selws->ns) {
		return;
	}

	selmon->selws->selcli = selmon->selws->stack[selmon->selws->ns-1];
	for (i = 0; i < selmon->selws->ns-1; i++) {
		XSetWindowBorder(dpy, selmon->selws->stack[i]->win, CBORDERNORM);
	}
	XSetWindowBorder(dpy, selmon->selws->selcli->win, CBORDERSEL);
	XSetInputFocus(dpy, selmon->selws->selcli->win, RevertToPointerRoot,
			CurrentTime);
}

void
updatemons(void)
{
	int i, n;
	XineramaScreenInfo *info;
	Monitor *mon;

	if (!XineramaIsActive(dpy)) {
		warn("Xinerama is not active");
		selmon = initmon();
		selmon->sx = selmon->sy = 0;
		selmon->sw = DisplayWidth(dpy, screen);
		selmon->sh = DisplayHeight(dpy, screen);
		attachmon(selmon);
		return;
	}

	info = XineramaQueryScreens(dpy, &n);
	debug("%u elements in the info array", n);
	for (i = 0; i < n; i++) {
		debug("info[%d]: %ux%u%+d%+d", i, info[i].x_org, info[i].y_org,
				info[i].width, info[i].height);
	}
	n = unifyscreens(&info, n);
	debug("%u elemenŧs in the unified info array", n);
	for (i = 0; i < n; i++) {
		debug("info[%d]: %ux%u%+d%+d", i, info[i].x_org, info[i].y_org,
				info[i].width, info[i].height);
	}

	if (n < nmon) { /* screen detached */
		for (i = nmon-1; i >= n; i--) {
			detachmon(monitors[i]);
			free(monitors[i]); /* <<== segfault! */
		}
	}
	for (i = 0; i < n; i++) {
		if (i >= nmon) { /* screen attached */
			mon = initmon();
			attachmon(mon);
		}
	}

	/* TODO */

	XFree(info);
}

void
updatewsdmap(void)
{
	unsigned int i;

	if (!wsd.shown) {
		return;
	}

	for (i = 0; i < nws; i++) {
		updatewsdbox(workspaces[i]);
	}
	updatewsdbox(wsd.target);
	XSync(dpy, false);
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

void
updatewsdbar(XEvent *e, char const *name)
{
	Status status;
	char code[20];
	unsigned int i, count;
	KeySym keysym;
	Workspace *ws;
	bool special = false;

	/* if name is set, replace current buffer by name */
	if (name) {
		strncpy(wsd.barbuf, name, 256);
		wsd.barcur = strlen(wsd.barbuf);
		renderwsdbar();
		return;
	}

	/* if IM is filtering the current input (e.g. dead key), ignore it */
	if (XFilterEvent(e, wsd.barwin)) {
		return;
	}

	/* special keys */
	keysym = XLookupKeysym(&e->xkey, 0);
	if (keysym == XK_Return || (e->xkey.state&ControlMask && keysym == XK_j)) {
		special = true;
		togglewsd(NULL);
		if (e->xkey.state&ControlMask && keysym != XK_j) {
			if (strlen(wsd.barbuf) && locatews(&ws, NULL, wsd.target->x,
					wsd.target->y, NULL)) {
				strncpy(ws->name, wsd.barbuf, 255);
				ws->name[255] = 0;
			}
		} else {
			if (strlen(wsd.barbuf) && locatews(&ws, NULL, 0, 0, wsd.barbuf)) {
				setws(ws->x, ws->y);
			} else {
				setws(wsd.target->x, wsd.target->y);
			}
		}
	} else if (keysym == XK_Left && wsd.barcur) {
		special = true;
		wsd.barcur--;
	} else if (keysym == XK_Right && wsd.barcur < strlen(wsd.barbuf)) {
		special = true;
		wsd.barcur++;
	} else if (keysym==XK_Home || (e->xkey.state&ControlMask && keysym==XK_a)) {
		special = true;
		wsd.barcur = 0;
	} else if (keysym==XK_End || (e->xkey.state&ControlMask && keysym==XK_e)) {
		special = true;
		wsd.barcur = strlen(wsd.barbuf);
	} else if (e->xkey.state&(ControlMask|Mod1Mask)) {
		special = true;
	}
	if (special) {
		renderwsdbar();
		return;
	}

	/* get key value */
	count = Xutf8LookupString(wsd.ic, (XKeyPressedEvent *) e, code, 20,
			NULL, &status);
	if (!count) {
		return;
	}

	if (count == 1 && code[0] == 0x08) { /* backspace */
		if (wsd.barcur) {
			for (i = --wsd.barcur; i < strlen(wsd.barbuf); i++) {
				wsd.barbuf[i] = wsd.barbuf[i+1];
			}
		}
	} else if (count == 1 && code[0] == 0x7F) { /* delete */
		if (wsd.barcur < strlen(wsd.barbuf)) {
			for (i = wsd.barcur; i < strlen(wsd.barbuf); i++) {
				wsd.barbuf[i] = wsd.barbuf[i+1];
			}
		}
	} else if (strlen(wsd.barbuf)+count < 256) {
		for (i = strlen(wsd.barbuf)+count; i >= wsd.barcur+count; i--) {
			wsd.barbuf[i] = wsd.barbuf[i-count];
		}
		strncpy(wsd.barbuf+wsd.barcur, code, count);
		wsd.barcur += count;
	} else {
		warn("WSD bar: buffer is full");
	}

	renderwsdbar();
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

	if (!locate(NULL, &c, &pos, selmon->selws->selcli->win)) {
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
	arrange();
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

