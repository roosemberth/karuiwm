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

/* macros */
#define APPNAME "stwm"
#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))

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
enum { LEFT, RIGHT, UP, DOWN };

/* structs */

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
	Client **clients, **stack;
	Client *selcli;
	Window wsdbox;
	unsigned int nc, ns;
	unsigned int nmaster;
	float mfact;
	char name[256];
	int x, y;
} Workspace;

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
static void detachws(Workspace *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void focusin(XEvent *);
static void focusout(XEvent *);
static void grabbuttons(void);
static void grabkeys(void);
static void hide(Client *);
static void hidews(Workspace *);
static void init(void);
static void initbar(void);
static Client *initclient(Window, bool);
static void initfont(void);
static void initwsd(void);
static Window initwsdbox(void);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void killclient(Arg const *);
static bool locate(Workspace **, Client **, unsigned int *, Window);
static bool locatews(Workspace **, unsigned int *, int, int, char const *, unsigned int);
static void mapnotify(XEvent *);
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
static void resetws(Workspace *, int, int);
static void restart(Arg const *);
static void run(void);
static void scan(void);
static void setmfact(Arg const *);
static void setws(int, int);
static void setnmaster(Arg const *);
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
static void unmapnotify(XEvent *);
static void updatebar(void);
static void updatefocus(void);
static void updatewsd(void);
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
	[FocusIn]          = focusin,          /* 9*/
	[FocusOut]         = focusout,         /*10*/
	[KeyPress]         = keypress,         /* 2*/
	[KeyRelease]       = keyrelease,       /* 3*/
	[MapNotify]        = mapnotify,        /*19*/
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
static unsigned int sw, sh;         /* screen dimensions */
static Window barwin;               /* status bar window */
static unsigned int bx, by, bw, bh; /* status bar dimensions */
static Window root;                 /* root window */
static unsigned int nws;            /* number of workspaces */
static Workspace **workspaces;      /* list of workspaces */
static Workspace *selws;            /* selected workspace */
static unsigned int wx, wy, ww, wh; /* workspace dimensions */
static DC dc;                       /* drawing context */
static Cursor cursor[CURSOR_LAST];  /* cursors */
static WorkspaceDialog wsd;         /* workspace dialog */

/* configuration */
#include "config.h"

void
arrange(void)
{
	/* TODO use array of layouts */
	tile();
}

void
attach(Workspace *ws, Client *c)
{
	unsigned int i, pos;

	/* attach workspace if this is the first client */
	if (!ws->nc) {
		attachws(ws);
	}

	/* add to list */
	ws->clients = realloc(ws->clients, ++ws->nc*sizeof(Client *));
	if (!ws->clients) {
		die("could not allocate %d bytes for client list",
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
attachws(Workspace *ws)
{
	workspaces = realloc(workspaces, ++nws*sizeof(Workspace *));
	if (!workspaces) {
		die("could not allocate %d bytes for workspace list",
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
	bool selfound = false;

	for (i = 0; i < nws; i++) {
		for (j = 0; j < workspaces[i]->nc; j++) {
			free(workspaces[i]->clients[j]);
		}
		free(workspaces[i]->clients);
		if (workspaces[i] == selws) {
			selfound = true;
		}
		free(workspaces[i]);
	}
	free(workspaces);
	if (!selfound) {
		free(selws);
	}
	XDestroyWindow(dpy, barwin);
	XDestroyWindow(dpy, wsd.barwin);
	XDestroyWindow(dpy, wsd.target->wsdbox);
	free(wsd.target);
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

	Workspace *ws;
	Client *c;
	XWindowAttributes wa;

	/* ignore unhandled windows */
	if (!locate(&ws, &c, NULL, e->xconfigure.window)) {
		return;
	}

	/* ignore buggy windows or windows with override_redirect */
	if (!XGetWindowAttributes(dpy, c->win, &wa)) {
		warn("XGetWindowAttributes() failed for window %d", c->win);
		return;
	}

	/* force client dimensions */
	if (wa.x != c->x || wa.y != c->y || wa.width != c->w || wa.height != c->h) {
		XResizeWindow(dpy, c->win, c->w, c->h);
	}

	/* make sure client stays invisible if not on current workspace */
	if (ws != selws) {
		hide(c);
	}
}

void
configurerequest(XEvent *e)
{
	debug("configurerequest(%d)", e->xconfigurerequest.window);

	XConfigureRequestEvent *ev = &e->xconfigurerequest;

	XWindowChanges wc = {
		.x = ev->x,
		.y = ev->y,
		.width = ev->width,
		.height = ev->height,
		.border_width = ev->border_width,
		.sibling = ev->above,
		.stack_mode = ev->detail,
	};
	XConfigureWindow(dpy, ev->window, ev->value_mask, &wc);
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

	/* remove the layout if the last client was removed */
	if (!ws->nc) {
		detachws(ws);
	}
}

void
detachws(Workspace *ws)
{
	unsigned int i;

	if (!locatews(&ws, &i, ws->x, ws->y, NULL, 0)) {
		warn("attempt to detach non-existing workspace");
		return;
	}

	nws--;
	for (; i < nws; i++) {
		workspaces[i] = workspaces[i+1];
	}
	workspaces = realloc(workspaces, nws*sizeof(Workspace *));
	if (!workspaces && nws) {
		die("could not allocate %d bytes for workspaces",
				nws*sizeof(Workspace *));
	}
}

void
enternotify(XEvent *e)
{
	debug("enternotify(%d)", e->xcrossing.window);

	unsigned int pos;
	Workspace *ws;
	Client *c;

	if (!locate(&ws, &c, &pos, e->xcrossing.window) || ws != selws) {
		warn("attempt to enter unhandled/invisible window %d",
				e->xcrossing.window);
		return;
	}

	push(selws, c);
	updatefocus();
}

void
expose(XEvent *e)
{
	debug("expose(%d)", e->xexpose.window);

	unsigned int i;

	if (e->xexpose.window == barwin) {
		renderbar();
	}
	for (i = 0; i < nws; i++) {
		if (e->xexpose.window == workspaces[i]->wsdbox) {
			renderwsdbox(workspaces[i]);
		}
	}
	if (e->xexpose.window == selws->wsdbox) {
		renderwsdbox(selws);
	}
	if (e->xexpose.window == wsd.target->wsdbox) {
		renderwsdbox(wsd.target);
	}
}

void
focusin(XEvent *e)
{
	debug("focusin(%d)", e->xfocus.window);
	/* TODO */
}

void
focusout(XEvent *e)
{
	debug("focusout(%d)", e->xfocus.window);
	/* TODO */
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
	XMoveWindow(dpy, c->win, -c->w-2*borderwidth, c->y);
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
init(void)
{
	XSetWindowAttributes wa;

	/* kill zombies */
	sigchld(0);

	/* locale */
	if (!setlocale(LC_ALL, "") || !XSupportsLocale()) {
		die("could not set locale");
	}

	/* X */
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	xerrorxlib = XSetErrorHandler(xerror);
	dc.gc = XCreateGC(dpy, root, 0, NULL);

	/* cursor/input */
	cursor[CURSOR_NORMAL] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CURSOR_RESIZE] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CURSOR_MOVE] = XCreateFontCursor(dpy, XC_fleur);
	wa.cursor = cursor[CURSOR_NORMAL];
	wa.event_mask = SubstructureNotifyMask|SubstructureRedirectMask|KeyPressMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);

	/* input */
	grabbuttons();
	grabkeys();

	/* workspace (TODO duplication) */
	selws = malloc(sizeof(Workspace));
	if (!selws) {
		die("could not allocate %d bytes for workspace", sizeof(Workspace));
	}
	resetws(selws, 0, 0);

	/* status bar */
	initfont();
	initbar();
	initwsd();
}

void
initbar(void)
{
	XSetWindowAttributes wa;

	bx = 0;
	by = 0; /* TODO allow bar to be at bottom */
	bh = dc.font.height + 2;
	bw = sw;
	wx = 0;
	wy = bh;
	ww = sw;
	wh = sh-bh;

	wa.override_redirect = true;
	wa.background_pixmap = ParentRelative;
	wa.event_mask = ExposureMask;
	barwin = XCreateWindow(dpy, root, bx, by, bw, bh, 0,
			DefaultDepth(dpy, screen), CopyFromParent,
			DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	XMapRaised(dpy, barwin);
	updatebar();
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
	if (viewable && wa.map_state != IsViewable) {
		return NULL;
	}

	/* create client */
	c = malloc(sizeof(Client));
	if (!c) {
		die("could not allocate new client (%d bytes)", sizeof(Client));
	}
	c->win = win;
	return c;
}

void
initfont(void)
{
	XFontStruct **xfonts;
	char **xfontnames;
	char *def, **missing;
	int n;

	dc.font.xfontset = XCreateFontSet(dpy, font, &missing, &n, &def);
	if (missing) {
		while (n--) {
			warn("missing fontset: %s", missing[n]);
		}
		XFreeStringList(missing);
	}

	/* if fontset load is successful, get information; otherwise dummy font */
	if (dc.font.xfontset) {
		dc.font.ascent = dc.font.descent = 0;
		//XExtentsOfFontSet(dc.font.xfontset); /* TODO why? */
		n = XFontsOfFontSet(dc.font.xfontset, &xfonts, &xfontnames);
		while (n--) {
			dc.font.ascent = MAX(dc.font.ascent, xfonts[n]->ascent);
			dc.font.descent = MAX(dc.font.descent, xfonts[n]->descent);
		}
	} else {
		dc.font.xfontstruct = XLoadQueryFont(dpy, font);
		if (!dc.font.xfontstruct) {
			die("cannot load font '%s'", font);
		}
		dc.font.ascent = dc.font.xfontstruct->ascent;
		dc.font.descent = dc.font.xfontstruct->descent;
	}
	dc.font.height = dc.font.ascent + dc.font.descent;
}

void
initwsd(void)
{
	/* WSD data */
	wsd.rad = wsdradius;
	wsd.rows = 2*wsd.rad+1;
	wsd.cols = 2*wsd.rad+1;
	wsd.w = ww/wsd.cols;
	wsd.h = wh/wsd.rows;
	wsd.shown = false;

	/* WSD window informations */
	wsd.wa.override_redirect = true;
	wsd.wa.background_pixmap = ParentRelative;
	wsd.wa.event_mask = ExposureMask;

	/* target box */
	wsd.target = malloc(sizeof(Workspace));
	if (!wsd.target) {
		die("could not allocate %d bytes for workspace", sizeof(Workspace));
	}
	resetws(wsd.target, 0, 0);
	wsd.target->wsdbox = initwsdbox();
	XMapRaised(dpy, wsd.target->wsdbox);

	/* bar */
	wsd.barwin = XCreateWindow(dpy, root, -bw, -bh, bw, bh, 0,
			DefaultDepth(dpy, screen), CopyFromParent,
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

Window
initwsdbox(void)
{
	return XCreateWindow(dpy, root, -wsd.w, -wsd.h,
			wsd.w-2*wsdborder, wsd.h-2*wsdborder, 0, DefaultDepth(dpy, screen),
			CopyFromParent, DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wsd.wa);
}
void
keypress(XEvent *e)
{
	debug("keypress()");

	unsigned int i;
	KeySym keysym = XLookupKeysym(&e->xkey, 0);

	if (!wsd.shown) {
		for (i = 0; i < LENGTH(keys); i++) {
			if (e->xkey.state == keys[i].mod && keysym == keys[i].key &&
					keys[i].func) {
				keys[i].func(&keys[i].arg);
				return;
			}
		}
	} else {
		for (i = 0; i < LENGTH(wsdkeys); i++) {
			if (e->xkey.state == wsdkeys[i].mod && keysym == wsdkeys[i].key &&
					wsdkeys[i].func) {
				wsdkeys[i].func(&wsdkeys[i].arg);
				updatewsd();
				return;
			}
		}
		updatewsdbar(e, NULL);
	}
}

void
keyrelease(XEvent *e)
{
	debug("keyrelease()");
}

void
killclient(Arg const *arg)
{
	XKillClient(dpy, selws->selcli->win);
	XSync(dpy, false);
}

bool
locate(Workspace **ws, Client **c, unsigned int *pos, Window w)
{
	unsigned int i, j;

	/* search current workspace */
	for (i = 0; i < selws->nc; i++) {
		if (selws->clients[i]->win == w) {
			if (ws) *ws = selws;
			if (c) *c = selws->clients[i];
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
locatews(Workspace **ws, unsigned int *pos, int x, int y, char const *name,
		unsigned int namelen)
{
	unsigned int i;
	for (i = 0; i < nws; i++) {
		if (name) {
			if (!strncmp(name, workspaces[i]->name, namelen)) {
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
mapnotify(XEvent *e)
{
	debug("mapnotify(%d)", e->xmap.window);
	/* TODO root window may generate mapnotify()s */
}

void
maprequest(XEvent *e)
{
	debug("maprequest(%d)", e->xmaprequest.window);

	Client *c = initclient(e->xmap.window, false);
	if (c) {
		attach(selws, c);
		arrange();
		XSetWindowBorderWidth(dpy, c->win, borderwidth);
		XMapWindow(dpy, c->win);
		updatefocus();
		updatebar();
	}
	updatewsd();
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
	Client *c = selws->selcli;
	detach(c);
	stepws(arg);
	attach(selws, c);
	arrange();
	updatefocus();
	updatewsd();
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

	locatews(&src, NULL, wsd.target->x, wsd.target->y, NULL, 0);
	locatews(&dst, NULL, x, y, NULL, 0);
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
	c->x = x;
	c->y = y;
	c->w = w;
	c->h = h;
	XMoveResizeWindow(dpy, c->win, x, y, w, h);
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
				die("could not allocate %d bytes for stack",
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
	}

	pop(ws, c);
	ws->stack = realloc(ws->stack, ++ws->ns*sizeof(Client *));
	if (!ws->stack) {
		die("could not allocated %d bytes for stack", ws->ns*sizeof(Client *));
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

	/* no random name available */
	strcpy(ws->name, "");
}

void
renderbar(void)
{
	char *empty = "<empty>";
	unsigned int len = strlen(selws->name);

	XSetForeground(dpy, dc.gc, cbordernorm);
	XFillRectangle(dpy, barwin, dc.gc, 0, 0, bw, bh);
	XSetForeground(dpy, dc.gc, 0x888888);
	XDrawString(dpy, barwin, dc.gc, 0, dc.font.ascent+1,
			len ? selws->name : empty, strlen(len ? selws->name : empty));
	XSync(dpy, false);
}

void
renderwsdbar(void)
{
	int cursorx;

	/* text */
	XSetForeground(dpy, dc.gc, cbordernorm);
	XFillRectangle(dpy, wsd.barwin, dc.gc, 0, 0, bw, bh);
	XSetForeground(dpy, dc.gc, wsdcsel);
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
	Workspace *ws2;
	char name[256];

	/* data */
	if (ws == wsd.target && locatews(&ws2, NULL, ws->x, ws->y, NULL, 0)) {
		strcpy(name, ws2->name);
	} else if (strlen(ws->name)) {
		strcpy(name, ws->name);
	} else {
		strcpy(name, "");
	}

	/* border */
	XSetWindowBorderWidth(dpy, ws->wsdbox, wsdborder);
	XSetWindowBorder(dpy, ws->wsdbox, ws == selws ? wsdcbordersel
			: ws == wsd.target ? wsdcbordertarget : wsdcbordernorm);

	/* background */
	XSetForeground(dpy, dc.gc, ws == selws ? wsdcbgsel
			: ws == wsd.target ? wsdcbgtarget : wsdcbgnorm);
	XFillRectangle(dpy, ws->wsdbox, dc.gc, 0, 0, wsd.w, wsd.h);

	/* text */
	XSetForeground(dpy, dc.gc, ws == selws ? wsdcsel
			: ws == wsd.target ? wsdctarget : wsdcnorm);
	Xutf8DrawString(dpy, ws->wsdbox, dc.font.xfontset, dc.gc, 2,
			dc.font.ascent+1, name, strlen(name));
}

void
resetws(Workspace *ws, int x, int y)
{
	ws->clients = selws->stack = NULL;
	ws->nc = selws->ns = 0;
	ws->x = x;
	ws->y = y;
	ws->mfact = mfact;
	ws->nmaster = nmaster;
	ws->name[0] = 0;
	ws->wsdbox = 0;
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
			attach(selws, c);
			updatefocus();
		}
	}
}

void
setmfact(Arg const *arg)
{
	selws->mfact = MAX(0.1, MIN(0.9, selws->mfact+arg->f));
	arrange();
}

void
setws(int x, int y)
{
	Workspace *next;

	if (locatews(&next, NULL, x, y, NULL, 0)) {
		if (!selws->nc) {
			free(selws);
		} else {
			hidews(selws);
		}
		selws = next;
	} else {
		if (selws->nc) {
			hidews(selws);
			selws = malloc(sizeof(Workspace));
			if (!selws) {
				die("could not allocate %d bytes for workspace",
						sizeof(Workspace));
			}
		}
		resetws(selws, x, y);
	}
	arrange();
	updatefocus();
	updatebar();
}

void
setnmaster(Arg const *arg)
{
	if (!selws->nmaster && arg->i < 0) {
		return;
	}
	selws->nmaster = selws->nmaster+arg->i;
	arrange();
}

void
shift(Arg const *arg)
{
	unsigned int pos;

	if (selws->ns < 2) {
		return;
	}
	if (!locate(NULL, NULL, &pos, selws->selcli->win)) {
		warn("attempt to shift non-existent window %d", selws->selcli->win);
	}
	selws->clients[pos] = selws->clients[(pos+selws->nc+arg->i)%selws->nc];
	selws->clients[(pos+selws->nc+arg->i)%selws->nc] = selws->selcli;

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

	if (!selws->nc) {
		return;
	}
	locate(NULL, NULL, &pos, selws->selcli->win);
	push(selws, selws->clients[(pos+selws->nc+arg->i)%selws->nc]);
	updatefocus();
}

void
stepwsdbox(Arg const *arg)
{
	Workspace *ws;
	char buf[256];

	switch (arg->i) {
		case LEFT:  wsd.target->x--; break;
		case RIGHT: wsd.target->x++; break;
		case UP:    wsd.target->y--; break;
		case DOWN:  wsd.target->y++; break;
	}
	if (locatews(&ws, NULL, wsd.target->x, wsd.target->y, NULL, 0)) {
		strncpy(buf, ws->name, 255);
		buf[255] = 0;
	} else {
		buf[0] = 0;
	}
	updatewsdbar(NULL, buf);
}

void
stepws(Arg const *arg)
{
	int x=selws->x, y=selws->y;

	switch (arg->i) {
		case LEFT:  x = selws->x-1; break;
		case RIGHT: x = selws->x+1; break;
		case UP:    y = selws->y-1; break;
		case DOWN:  y = selws->y+1; break;
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

void
tile(void)
{
	unsigned int ncm, i, w, h;
	int x;

	if (!selws->nc) {
		return;
	}

	/* draw master area */
	ncm = MIN(selws->nmaster, selws->nc);
	if (ncm) {
		x = wx;
		w = selws->nmaster >= selws->nc ? ww : selws->mfact*ww;
		h = wh/ncm;
		for (i = 0; i < ncm; i++) {
			place(selws->clients[i], x, wy+i*h, w-2*borderwidth,
					h-2*borderwidth);
		}
	}
	if (ncm == selws->nc) {
		return;
	}

	/* draw stack area */
	x = ncm ? selws->mfact*ww : 0;
	w = ncm ? ww-x : ww;
	h = wh/(selws->nc-ncm);
	for (i = ncm; i < selws->nc; i++) {
		place(selws->clients[i], x, wy+(i-ncm)*h, w-2*borderwidth,
				h-2*borderwidth);
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
		if (selws->wsdbox) {
			XDestroyWindow(dpy, selws->wsdbox);
			selws->wsdbox = 0;
		}
		/* hide target box and input bar */
		XMoveWindow(dpy, wsd.target->wsdbox, -wsd.w, -wsd.h);
		XMoveWindow(dpy, wsd.barwin, -bh, -bw);
		wsd.shown = false;
		XUngrabKeyboard(dpy, selws->wsdbox);
		grabkeys();
		updatefocus();
		return;
	}

	/* create a box for all mapped + current workspace(s) and hide it */
	for (i = 0; i < nws; i++) {
		workspaces[i]->wsdbox = initwsdbox();
		XMapRaised(dpy, workspaces[i]->wsdbox);
	}
	if (!selws->wsdbox) {
		selws->wsdbox = initwsdbox();
		XMapRaised(dpy, selws->wsdbox);
	}
	XRaiseWindow(dpy, wsd.target->wsdbox);

	/* show input bar */
	XMoveWindow(dpy, wsd.barwin, bx, by);
	XRaiseWindow(dpy, wsd.barwin);
	wsd.barbuf[0] = 0;
	wsd.barcur = 0;
	renderwsdbar();

	/* initially selected workspace is the current workspace */
	wsd.target->x = selws->x;
	wsd.target->y = selws->y;
	wsd.shown = true;

	/* grab keyboard, now we're in WSD mode */
	XGrabKeyboard(dpy, selws->wsdbox, false, GrabModeAsync, GrabModeAsync,
			CurrentTime);

	/* initial update */
	updatewsd();
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
		if (ws == selws) {
			arrange();
			updatefocus();
		}
	}
	updatewsd();
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

	if (!selws->ns) {
		return;
	}

	/* unfocus all but the top of the stack */
	for (i = 0; i < selws->ns-1; i++) {
		XSetWindowBorder(dpy, selws->stack[i]->win, cbordernorm);
	}

	/* focus top of the stack */
	selws->selcli = selws->stack[selws->ns-1];
	XSetWindowBorder(dpy, selws->selcli->win, cbordersel);
	XSetInputFocus(dpy, selws->selcli->win, RevertToPointerRoot, CurrentTime);
}

void
updatewsd(void)
{
	unsigned int i;

	if (!wsd.shown) {
		return;
	}

	for (i = 0; i < nws; i++) {
		updatewsdbox(workspaces[i]);
	}
	updatewsdbox(selws);
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
	x = ww/2 - wsd.w*wsd.rad - wsd.w/2 + c*wsd.w + wx;
	y = wh/2 - wsd.h*wsd.rad - wsd.h/2 + r*wsd.h + wy;
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
					wsd.target->y, NULL, 0)) {
				strncpy(ws->name, wsd.barbuf, 255);
				ws->name[255] = 0;
			}
		} else {
			if (strlen(wsd.barbuf) && locatews(&ws, NULL, 0, 0, wsd.barbuf,
					strlen(wsd.barbuf))) {
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

	if (!selws->nc) {
		return;
	}

	if (!locate(NULL, &c, &pos, selws->selcli->win)) {
		warn("attempt to zoom non-existing window %d", selws->selcli->win);
		return;
	}

	if (!pos) {
		/* window is at the top */
		if (selws->nc > 1) {
			selws->clients[0] = selws->clients[1];
			selws->clients[1] = c;
			push(selws, selws->clients[0]);
			updatefocus();
		} else {
			return;
		}
	} else {
		/* window is somewhere else */
		for (i = pos; i > 0; i--) {
			selws->clients[i] = selws->clients[i-1];
		}
		selws->clients[0] = c;
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
	init();
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

