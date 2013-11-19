#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <sys/wait.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>

/* macros */
#ifdef DEBUG
#define debug(...) stdlog(stdout, "\033[34mDBG\033[0m "__VA_ARGS__)
#else
#define debug(...)
#endif
#define warn(...) stdlog(stderr, "\033[33mWRN\033[0m "__VA_ARGS__)
#define die(...) warn("\033[31mERR\033[0m "__VA_ARGS__); exit(EXIT_FAILURE)
#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))

/* enums */

enum { CURSOR_NORMAL, CURSOR_RESIZE, CURSOR_MOVE, CURSOR_LAST };
typedef enum { WSSTEP_LEFT, WSSTEP_RIGHT, WSSTEP_UP, WSSTEP_DOWN } WSStepDirection;

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
	unsigned int nc, ns;
	unsigned int nmaster;
	float mfact;
	char name[256];
	int x, y;
} Workspace;

typedef struct WorkspaceDialogBox WorkspaceDialogBox;
typedef struct {
	WorkspaceDialogBox *boxes;
	int nb;
	int w, h;
	int rows, cols, rad;
	bool shown;
} WorkspaceDialog;

struct WorkspaceDialogBox {
	Window win;
	bool shown;
};

/* functions */
static void arrange(void);
static void attach(Window);
static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void cleanup(void);
static void clientmessage(XEvent *);
static void configurerequest(XEvent *);
static void configurenotify(XEvent *);
static void createnotify(XEvent *);
static void destroynotify(XEvent *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void focusin(XEvent *);
static void focusstep(Arg const *);
static void grabbuttons(void);
static void grabkeys(void);
static void hide(Client *);
static void init(void);
static void initbar(void);
static void initdrawcontext(void);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void killclient(Arg const *);
static void mapnotify(XEvent *);
static void mappingnotify(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void pop(Workspace *, Client *);
static void propertynotify(XEvent *);
static void push(Workspace *, Client *);
static void quit(Arg const *);
static void restart(Arg const *);
static void run(void);
static void scan(void);
static void setmfact(Arg const *);
static void setnmaster(Arg const *);
static void shift(Arg const *);
static void sigchld(int);
static void spawn(Arg const *);
static void stdlog(FILE *, char const *, ...);
static void tile(void);
static void unmapnotify(XEvent *);
static void updatebar(void);
static void updatefocus(void);
static bool wintoclient(Workspace **, Client **, unsigned int *, Window);
static void ws_attach(Workspace *);
static void ws_baptise(Workspace *);
static void ws_detach(Workspace *);
static void ws_find(Workspace **, unsigned int *, int, int);
static void ws_hide(Workspace *);
static void ws_step(Arg const *);
static void wsd_toggle(Arg const *);
static void wsd_update(void);
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
static char *appname;               /* application name */
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
	unsigned int i = 0;

	/* disable EnterWindowMask, so the focus won't change at every rearrange */
	for (i = 0; i < selws->nc; i++) {
		XSelectInput(dpy, selws->clients[i]->win, 0);
	}
	tile();
	for (i = 0; i < selws->nc; i++) {
		XSelectInput(dpy, selws->clients[i]->win, EnterWindowMask);
	}
}

void
attach(Window w)
{
	Client *c;
	XWindowAttributes wa;

	/* don't manage junk windows */
	if (!XGetWindowAttributes(dpy, w, &wa)) {
		warn("XGetWindowAttributes() failed for window %d", w);
		return;
	}
	if (wa.override_redirect || wa.map_state != IsViewable) {
		return;
	}

	/* create client */
	c = malloc(sizeof(Client));
	if (!c) {
		die("could not allocate new client (%d bytes)", sizeof(Client));
	}

	/* attach workspace if this is the first client */
	if (!selws->nc) {
		ws_attach(selws);
	}

	/* add to list */
	selws->clients = realloc(selws->clients, ++selws->nc*sizeof(Client *));
	if (!selws->clients) {
		die("could not allocate %d bytes for client list",
				selws->nc*sizeof(Client *));
	}
	selws->clients[selws->nc-1] = c;

	/* configure */
	c->win = w;
	XSelectInput(dpy, c->win, EnterWindowMask);
	XSetWindowBorderWidth(dpy, c->win, borderwidth);

	/* update layout and focus */
	arrange();
	push(selws, c);
	updatefocus();

	/* update workspace dialog if present */
	wsd_update();
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
	unsigned int i;
	for (i = 0; i < selws->nc; i++) {
		free(selws->clients[i]);
	}
	free(selws);
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

	unsigned int pos;
	Workspace *ws;
	Client *c;
	XWindowAttributes wa;

	if (!wintoclient(&ws, &c, &pos, e->xconfigure.window)) {
		return;
	}

	/* correct the size of the window if necessary (size hints) */
	if (!XGetWindowAttributes(dpy, c->win, &wa)) {
		warn("XGetWindowAttributes(%d) failed", c->win);
		return;
	}
	if (wa.x != c->x || wa.y != c->y || wa.width != c->w || wa.height != c->h) {
		XResizeWindow(dpy, c->win, c->w-2*borderwidth, c->h-2*borderwidth);
	}

	/* make sure window stays invisible if not on current workspace */
	if (ws != selws) {
		hide(c);
	}
}

void
configurerequest(XEvent *e)
{
	debug("configurerequest(%d)", e->xconfigurerequest.window);
	/* TODO */
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
enternotify(XEvent *e)
{
	debug("enternotify(%d)", e->xcrossing.window);

	unsigned int pos;
	Workspace *ws;
	Client *c;

	if (!wintoclient(&ws, &c, &pos, e->xcrossing.window) || ws != selws) {
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

	if (e->xexpose.window == barwin) {
		updatebar();
	}
}

void
focusin(XEvent *e)
{
	debug("focusin(%d)", e->xfocus.window);
	/* TODO */
}

void
focusstep(Arg const *arg)
{
	unsigned int pos;

	if (!selws->nc) {
		return;
	}
	wintoclient(NULL, NULL, &pos, selws->selcli->win);
	push(selws, selws->clients[(pos+selws->nc+arg->i)%selws->nc]);
	updatefocus();
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
	XMoveWindow(dpy, c->win, -c->w, c->y);
}

void
init(void)
{
	XSetWindowAttributes wa;

	sigchld(0);
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	xerrorxlib = XSetErrorHandler(xerror);

	/* cursor/input */
	cursor[CURSOR_NORMAL] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CURSOR_RESIZE] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CURSOR_MOVE] = XCreateFontCursor(dpy, XC_fleur);
	wa.cursor = cursor[CURSOR_NORMAL];
	wa.event_mask = SubstructureNotifyMask|KeyPressMask;
	XChangeWindowAttributes(dpy, root, CWEventMask|CWCursor, &wa);

	/* input */
	grabbuttons();
	grabkeys();

	/* workspace (TODO duplication) */
	selws = malloc(sizeof(Workspace));
	if (!selws) {
		die("could not allocate %d bytes for workspace", sizeof(Workspace));
	}
	selws->clients = selws->stack = NULL;
	selws->nc = selws->ns = 0;
	selws->x = selws->y = 0;
	selws->nmaster = nmaster;
	selws->mfact = mfact;
	selws->name[0] = 0;

	/* status bar */
	initdrawcontext();
	initbar();

	/* workspace dialog */
	wsd.rad = wsdradius;
	wsd.rows = 2*wsd.rad+1;
	wsd.cols = 2*wsd.rad+1;
	wsd.w = ww/wsd.cols;
	wsd.h = wh/wsd.rows;
	wsd.nb = wsd.rows*wsd.cols;
	wsd.boxes = calloc(wsd.nb, sizeof(WorkspaceDialogBox));
	wsd.shown = false;
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
	XMapRaised(dpy, barwin); /* TODO find out what this exactly does */
	updatebar();
}

void
initdrawcontext(void)
{
	dc.gc = XCreateGC(dpy, root, 0, NULL);
	dc.font.xfontstruct = XLoadQueryFont(dpy, font);
	if (!dc.font.xfontstruct) {
		die("cannot load font '%s'", font);
	}
	dc.font.ascent = dc.font.xfontstruct->ascent;
	dc.font.descent = dc.font.xfontstruct->descent;
	dc.font.height = dc.font.ascent + dc.font.descent;
}

void
keypress(XEvent *e)
{
	debug("keypress()");

	unsigned int i;
	KeySym keysym = XLookupKeysym(&e->xkey, 0);

	for (i = 0; i < LENGTH(keys); i++) {
		if (e->xkey.state == keys[i].mod && keysym == keys[i].key &&
				keys[i].func) {
			keys[i].func(&keys[i].arg);
		}
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

void
mapnotify(XEvent *e)
{
	debug("mapnotify(%d)", e->xmap.window);

	attach(e->xmap.window);
}

void
maprequest(XEvent *e)
{
	debug("maprequest(%d)", e->xmaprequest.window);
	/* TODO */
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

	if (!XQueryTree(dpy, root, &r, &p, &wins, &nwins)) {
		warn("XQueryTree() failed");
		return;
	}
	for (i = 0; i < nwins; i++) {
		attach(wins[i]); /* manage() will do the necessary checks */
	}
}

void
setmfact(Arg const *arg)
{
	selws->mfact = MAX(0.1, MIN(0.9, selws->mfact+arg->f));
	arrange();
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
	if (!wintoclient(NULL, NULL, &pos, selws->selcli->win)) {
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
	pid_t pid = vfork();
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
			date->tm_hour, date->tm_min, date->tm_sec, appname);

	/* message */
	va_start(args, format);
	vfprintf(f, format, args);
	va_end(args);

	fprintf(f, "\n");
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
			selws->clients[i]->x = x;
			selws->clients[i]->y = wy+i*h;
			selws->clients[i]->w = w;
			selws->clients[i]->h = h;
			XMoveResizeWindow(dpy, selws->clients[i]->win,
					selws->clients[i]->x, selws->clients[i]->y,
					selws->clients[i]->w-2*borderwidth,
					selws->clients[i]->h-2*borderwidth);
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
		selws->clients[i]->x = x;
		selws->clients[i]->y = wy+(i-ncm)*h;
		selws->clients[i]->w = w;
		selws->clients[i]->h = h;
		XMoveResizeWindow(dpy, selws->clients[i]->win,
				selws->clients[i]->x, selws->clients[i]->y,
				selws->clients[i]->w-2*borderwidth,
				selws->clients[i]->h-2*borderwidth);
	}
}

void
unmapnotify(XEvent *e)
{
	debug("unmapnotify(%d)", e->xunmap.window);

	unsigned int i;
	Client *c;
	Workspace *ws;

	if (!wintoclient(&ws, &c, &i, e->xunmap.window)) {
		return;
	}

	/* remove from stack */
	pop(ws, c);

	/* remove client */
	free(c);
	ws->nc--;
	for (; i < ws->nc; i++) {
		ws->clients[i] = ws->clients[i+1];
	}

	/* update layout if the client was removed from the current workspace */
	if (ws == selws) {
		arrange();
		updatefocus();
	}

	/* remove the layout if the last client was removed */
	if (!ws->nc) {
		ws_detach(ws);
	}

	/* update workspace dialog if present */
	wsd_update();
}

void
updatebar(void)
{
	char name[256];
	if (strlen(selws->name)) {
		strncpy(name, selws->name, 256);
	} else {
		snprintf(name, 256, "[%d|%d]", selws->x, selws->y);
	}
	name[255] = 0;

	XSetForeground(dpy, dc.gc, cbordernorm);
	XFillRectangle(dpy, barwin, dc.gc, 0, 0, bw, bh);
	XSetForeground(dpy, dc.gc, cbordersel);
	XDrawString(dpy, barwin, dc.gc, 0, dc.font.ascent+1, name, strlen(name));
	XSync(dpy, false);
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

bool
wintoclient(Workspace **ws, Client **c, unsigned int *pos, Window w)
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

void
ws_attach(Workspace *ws)
{
	workspaces = realloc(workspaces, ++nws*sizeof(Workspace *));
	if (!workspaces) {
		die("could not allocate %d bytes for workspace list",
				nws*sizeof(Workspace *));
	}
	workspaces[nws-1] = ws;
	ws_baptise(ws);
}

void
ws_baptise(Workspace *ws)
{
	unsigned int n, w;
	bool found;
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
	strcpy(ws->name, "");
}

void
ws_detach(Workspace *ws)
{
	unsigned int i;

	ws_find(&ws, &i, ws->x, ws->y);
	if (!ws) {
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
ws_find(Workspace **ws, unsigned int *pos, int x, int y)
{
	unsigned int i;
	for (i = 0; i < nws; i++) {
		if (workspaces[i]->x == x && workspaces[i]->y == y) {
			if (ws) *ws = workspaces[i];
			if (pos) *pos = i;
			return;
		}
	}
	if (ws) *ws = NULL;
}

void
ws_hide(Workspace *ws)
{
	unsigned int i;
	for (i = 0; i < ws->nc; i++) {
		hide(ws->clients[i]);
	}
}

void
ws_step(Arg const *arg)
{
	Workspace *next;
	int x=selws->x, y=selws->y;

	switch (arg->i) {
		case WSSTEP_LEFT:  x = selws->x-1; break;
		case WSSTEP_RIGHT: x = selws->x+1; break;
		case WSSTEP_UP:    y = selws->y-1; break;
		case WSSTEP_DOWN:  y = selws->y+1; break;
	}

	ws_find(&next, NULL, x, y);
	if (next) {
		if (!selws->nc) {
			free(selws);
		} else {
			ws_hide(selws);
		}
		selws = next;
	} else {
		if (selws->nc) {
			ws_hide(selws);
			selws = malloc(sizeof(Workspace));
			if (!selws) {
				die("could not allocate %d bytes for workspace",
						sizeof(Workspace));
			}
		}
		selws->clients = selws->stack = NULL;
		selws->nc = selws->ns = 0;
		selws->x = x;
		selws->y = y;
		selws->mfact = mfact;
		selws->nmaster = nmaster;
		selws->name[0] = 0;
	}
	arrange();
	updatefocus();
	updatebar();
	wsd_update();
}

void
wsd_box(Workspace *ws)
{
	int r, c, pos, x, y;
	char name[256];
	XSetWindowAttributes wa;

	/* normalise for grid */
	c = ws->x + wsd.rad - selws->x;
	r = ws->y + wsd.rad - selws->y;
	pos = r*wsd.cols + c%wsd.cols;
	if (wsd.boxes[pos].shown) {
		return;
	}
	x = ww/2 - wsd.w*wsd.rad - wsd.w/2 + c*wsd.w;
	y = wh/2 - wsd.h*wsd.rad - wsd.h/2 + r*wsd.h;

	/* create window */
	wa.override_redirect = true;
	wa.background_pixmap = ParentRelative;
	wa.event_mask = ExposureMask;
	wsd.boxes[pos].win = XCreateWindow(dpy, root, x, y, wsd.w, wsd.h, 0,
			DefaultDepth(dpy, screen),
			CopyFromParent, DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wa);
	XMapRaised(dpy, wsd.boxes[pos].win);

	/* fill with data */
	if (strlen(ws->name)) {
		strncpy(name, ws->name, 256);
	} else {
		snprintf(name, 256, "[%d|%d]", ws->x, ws->y);
	}
	XSetForeground(dpy, dc.gc, ws == selws ? cbordersel : cbordernorm);
	XFillRectangle(dpy, wsd.boxes[pos].win, dc.gc, 0, 0, wsd.w, wsd.h);
	XSetForeground(dpy, dc.gc, ws == selws ? cbordernorm : cbordersel);
	XDrawString(dpy, wsd.boxes[pos].win, dc.gc, 2, dc.font.ascent+1,
			name, strlen(name));
	XSync(dpy, false);

	/* end */
	wsd.boxes[pos].shown = true;
}

void
wsd_toggle(Arg const *arg)
{
	int i;
	Workspace *ws;

	/* if map is shown, just remove it */
	if (wsd.shown) {
		for (i = 0; i < wsd.nb; i++) {
			if (wsd.boxes[i].shown) {
				XDestroyWindow(dpy, wsd.boxes[i].win);
				wsd.boxes[i].shown = false;
			}
		}
		wsd.shown = false;
	} else {
		for (i = 0; i < nws; i++) {
			ws = workspaces[i];
			if (ws->x + wsd.rad < selws->x || ws->x - wsd.rad > selws->x ||
					ws->y + wsd.rad < selws->y || ws->y - wsd.rad > selws->y) {
				continue;
			}
			wsd_box(ws);
		}
		wsd_box(selws);
		wsd.shown = true;
	}
}

void
wsd_update(void)
{
	if (!wsd.shown) {
		return;
	}

	wsd_toggle(NULL);
	wsd_toggle(NULL);
}

int
xerror(Display *dpy, XErrorEvent *ee)
{
	char es[256];

	/* only display error on this error instead of crashing */
	if (ee->error_code == BadWindow) {
		XGetErrorText(dpy, ee->error_code, es, 256);
		warn("%d: %s (after request %d)",
				ee->error_code, es, ee->error_code);
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

	if (!wintoclient(NULL, &c, &pos, selws->selcli->win)) {
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
	appname = argv[0];
	if (!(dpy = XOpenDisplay(NULL))) {
		die("could not open X");
	}
	stdlog(stdout, "starting ...");
	init();
	scan();
	custom_startup();
	run();
	custom_shutdown();
	cleanup();
	XCloseDisplay(dpy);
	stdlog(stdout, "shutting down ...");
	if (restarting) {
		execl("stwm", appname, NULL); /* TODO change to execlp */
	}
	return EXIT_SUCCESS;
}

