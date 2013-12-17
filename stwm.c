#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/select.h>
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
#define INTERSECT(MON, X, Y, W, H) \
	((MAX(0, MIN((X)+(W),(MON)->x+(MON)->w) - MAX((MON)->x, X))) * \
	 (MAX(0, MIN((Y)+(H),(MON)->y+(MON)->h) - MAX((MON)->y, Y))))
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
enum DMenuState { DMENU_SPAWN, DMENU_RENAME, DMENU_VIEW, DMENU_INACTIVE };

typedef union Arg Arg;
typedef struct Button Button;
typedef struct Client Client;
typedef struct Key Key;
typedef struct Layout Layout;
typedef struct Monitor Monitor;
typedef struct Workspace Workspace;

union Arg {
	int i;
	float f;
	const void *v;
};

struct Button {
	unsigned int mod;
	unsigned int button;
	void (*func)(Arg const *);
	Arg const arg;
};

struct Client {
	int x, y, w, h;
	char name[256];
	Window win;
	bool floating;
	int basew, baseh, incw, inch;
};

struct {
	GC gc;
	struct {
		int ascent, descent, height;
		XFontStruct *xfontstruct;
		XFontSet xfontset;
	} font;
} dc;

struct Key {
	unsigned int mod;
	KeySym key;
	void (*func)(Arg const *);
	Arg const arg;
};

struct Layout {
	char const *icon;
	void (*func)(Monitor *);
};

struct Monitor {
	Workspace *selws;
	int x, y, w, h;     /* monitor dimensions */
	int bx, by, bw, bh; /* status bar dimensions */
	int wx, wy, ww, wh; /* workspace dimensions */
	struct {
		Window win;
		char buffer[256];
	} bar;
};

struct Workspace {
	Client **clients, **stack;
	Client *selcli;
	Window wsmbox;
	unsigned int nc, ns;
	unsigned int nmaster;
	float mfact;
	char name[256];
	int x, y;
	int ilayout;
};

struct {
	Workspace *target;
	XSetWindowAttributes wa;
	int w, h; /* size of a box */
	int rows, cols, rad;
	bool active;
	XIM im;
	XIC ic;
} wsm;

/* functions */
static void arrange(Monitor *);
static void attachclient(Workspace *, Client *);
static void attachws(Workspace *);
static void buttonpress(XEvent *);
static bool checksizehints(Client *, int, int, int *, int *);
static void cleanup(void);
static void clientmessage(XEvent *);
static void configurerequest(XEvent *);
static void configurenotify(XEvent *);
static void detachclient(Client *);
static void detachmon(Monitor *);
static void detachws(Workspace *);
static void dmenu(Arg const *);
static void dmenueval(void);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void focusin(XEvent *);
static int gettiled(Client ***, Monitor *);
static void grabbuttons(Client *, bool);
static void grabkeys(void);
static void hideclient(Client *);
static void hidews(Workspace *);
static void initbar(Monitor *);
static Client *initclient(Window, bool);
static Monitor *initmon(void);
static Workspace *initws(int, int);
static Window initwsmbox(void);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void killclient(Arg const *);
static bool locateclient(Workspace **, Client **, unsigned int *, Window const);
static bool locatemon(Monitor **, unsigned int *, Workspace const *);
static Monitor *locaterect(int, int, int, int);
static bool locatews(Workspace **, unsigned int *, int, int, char const *);
static void mappingnotify(XEvent *);
static void maprequest(XEvent *);
static void moveclient(Arg const *);
static void movemouse(Arg const *);
static void moveresize(Monitor *, Client *, int, int, int, int);
static void movews(Arg const *);
static void pop(Workspace *, Client *);
static void propertynotify(XEvent *);
static void push(Workspace *, Client *);
static void quit(Arg const *);
static void reltoxy(int *, int *, Workspace *, int);
static void renamews(Workspace *, char const *);
static void renderbar(Monitor *);
static void renderwsmbox(Workspace *);
static void resizemouse(Arg const *);
static void restart(Arg const *);
static void run(void);
static void savesession(Arg const *arg);
static void scan(void);
static void setclientmask(Monitor *, bool);
static void setmfact(Arg const *);
static void setpad(Arg const *);
static void setup(void);
static void setupfont(void);
static void setupwsm(void);
static void setnmaster(Arg const *);
static void setws(Monitor *, int, int);
static void shift(Arg const *);
static void sigchld(int);
static void spawn(Arg const *);
static void stdlog(FILE *, char const *, ...);
static void stepfocus(Arg const *);
static void steplayout(Arg const *);
static void stepmon(Arg const *);
static void stepws(Arg const *);
static void stepwsmbox(Arg const *arg);
static void termclient(Client *);
static void termmon(Monitor *);
static void termws(Workspace *);
static void togglefloat(Arg const *);
static void togglepad(Arg const *);
static void togglewsm(Arg const *);
static size_t unifyscreens(XineramaScreenInfo **, size_t);
static void unmapnotify(XEvent *);
static void updatebar(Monitor *);
static void updatefocus(void);
static void updategeom(void);
static void updatemon(Monitor *, int, int, unsigned int, unsigned int);
static void updatesizehints(Client *);
static void updatewsm(void);
static void updatewsmbox(Workspace *);
static void viewmon(Monitor *);
static void viewws(Arg const *);
static int xerror(Display *, XErrorEvent *);
static int (*xerrorxlib)(Display *, XErrorEvent *);
static void zoom(Arg const *);

/* layouts */
static void bstack(Monitor *);
static void rstack(Monitor *);

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
static bool running, restarting;    /* application state */
static Display *dpy;                /* X display */
static int screen;                  /* screen */
static Window root;                 /* root window */
static Workspace **workspaces;      /* list of workspaces */
static unsigned int nws;            /* number of workspaces */
static Cursor cursor[CURSOR_LAST];  /* cursors */
static Monitor **monitors;          /* list of monitors */
static Monitor *selmon;             /* selected monitor */
static unsigned int nmon;           /* number of monitors */
static int dmenu_out;               /* dmenu's output file descriptor */
static enum DMenuState dmenu_state; /* dmenu's state */
static int nlayouts;                /* number of cyclable layouts */
static Client *pad;                 /* scratchpad window */
static Monitor *pad_mon;            /* monitor the scratchpad is currently on */

/* configuration */
#include "config.h"

/* function implementation */

void
arrange(Monitor *mon)
{
	unsigned int i;
	Client *c;

	setclientmask(mon, false);
	layouts[mon->selws->ilayout].func(mon);
	for (i = 0; i < mon->selws->nc; i++) {
		c = mon->selws->clients[i];
		if (c->floating) {
			XMoveWindow(dpy, c->win, mon->x+c->x, mon->y+c->y);
		}
	}
	setclientmask(mon, true);
}

void
attachclient(Workspace *ws, Client *c)
{
	unsigned int i, pos;
	Monitor *mon;
	XWindowAttributes wa;

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
	if (locatemon(&mon, NULL, ws)) {
		/* update coordinates if floating */
		if (c->floating) {
			XRaiseWindow(dpy, c->win);
			if (!XGetWindowAttributes(dpy, c->win, &wa)) {
				warn("XGetWindowAttributes() failed for window %d", c->win);
			} else {
				c->x = wa.x - mon->x;
				c->y = wa.y - mon->y;
			}
		}
		arrange(mon);
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
}

void
buttonpress(XEvent *e)
{
	unsigned int i;
	Client *c;
	Workspace *ws;
	Monitor *mon;

	if (!locateclient(&ws, &c, NULL, e->xbutton.window) ||
			!locatemon(&mon, NULL, ws)) {
		return;
	}
	if (mon != selmon) {
		viewmon(mon);
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
checksizehints(Client *c, int x, int y, int *w, int *h)
{
	int u;
	bool change = false;

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
	debug("cleanup");

	unsigned int i;
	Monitor *mon;
	Workspace *ws;
	Client *c;

	/* disable WSM */
	if (wsm.active) {
		togglewsm(NULL);
	}

	/* make monitors point nowhere (so all workspaces are removed) */
	for (i = 0; i < nmon; i++) {
		mon = monitors[i];
		if (!mon->selws->nc) {
			detachws(mon->selws);
			termws(mon->selws);
		}
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
	}
	termws(wsm.target);

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
	if (!FORCESIZE || (pad && ev->window == pad->win) ||
			!locateclient(NULL, &c, NULL, ev->window) || c->floating) {
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
		warn("attempt to detach non-existent client");
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

	for (i = 0; i < nmon; i++) {
		if (monitors[i] == mon) {
			nmon--;
			for (; i < nmon; i++) {
				monitors[i] = monitors[i+1];
			}
			monitors = realloc(monitors, nmon*sizeof(Monitor *));
			if (!monitors && nmon) {
				die("could not allocate %d bytes for monitor list",
						nmon*sizeof(Monitor *));
			}
			return;
		}
	}
	warn("attempt to detach non-existing monitor");
}

void
detachws(Workspace *ws)
{
	unsigned int i;

	if (!locatews(&ws, &i, ws->x, ws->y, NULL)) {
		warn("attempt to detach non-existent workspace");
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

	if (ws->wsmbox) {
		XDestroyWindow(dpy, ws->wsmbox);
		ws->wsmbox = 0;
		updatewsm();
	}
}

void
dmenu(Arg const *arg)
{
	int in[2], out[2], i;
	char const *cmd[LENGTH(dmenuargs)+3];

	/* assemble dmenu command */
	cmd[0] = arg->i == DMENU_SPAWN ? "dmenu_run" : "dmenu";
	for (i = 0; i < LENGTH(dmenuargs); i++) {
		cmd[i+1] = dmenuargs[i];
	}
	cmd[i] = "-p";
	switch (arg->i) {
		case DMENU_VIEW:   cmd[i+1] = PROMPT_CHANGE; break;
		case DMENU_RENAME: cmd[i+1] = PROMPT_RENAME; break;
		case DMENU_SPAWN:  cmd[i+1] = PROMPT_SPAWN;  break;
	}
	cmd[i+2] = NULL;

	/* create pipes */
	if (arg->i != DMENU_SPAWN && (pipe(in) < 0 || pipe(out) < 0)) {
		die("could not create pipes for dmenu");
	}

	/* child */
	if (fork() == 0) {
		if (arg->i != DMENU_SPAWN) {
			close(in[1]);
			close(out[0]);
			dup2(in[0], STDIN_FILENO);
			dup2(out[1], STDOUT_FILENO);
			close(in[0]);
			close(out[1]);
		}
		execvp(cmd[0], (char *const *) cmd);
		die("failed to execute %s", cmd[0]);
	}

	/* parent */
	if (arg->i == DMENU_SPAWN) {
		return;
	}
	close(in[0]);
	close(out[1]);
	if (arg->i == DMENU_VIEW) {
		for (i = 0; i < nws; i++) {
			write(in[1], workspaces[i]->name, strlen(workspaces[i]->name));
			write(in[1], "\n", 1);
		}
	}
	close(in[1]);
	dmenu_state = arg->i;
	dmenu_out = out[0];
}

void
dmenueval(void)
{
	int ret;
	char buf[256];
	Workspace *ws;

	ret = read(dmenu_out, buf, 256);
	if (ret < 0) {
		die("failed to read from dmenu file descriptor");
	}
	if (ret > 0) {
		buf[ret-1] = 0;
		switch (dmenu_state) {
			case DMENU_RENAME:
				renamews(selmon->selws, buf);
				updatebar(selmon);
				break;
			case DMENU_VIEW:
				if (locatews(&ws, NULL, 0, 0, buf)) {
					setws(selmon, ws->x, ws->y);
				}
				break;
			default:
				warn("unknown dmenu state: %u", dmenu_state);
		}
	}
	close(dmenu_out);
	dmenu_state = DMENU_INACTIVE;
}

void
enternotify(XEvent *e)
{
	debug("enternotify(%d)", e->xcrossing.window);

	unsigned int pos;
	Workspace *ws;
	Monitor *mon;
	Client *c;

	if (pad && e->xcrossing.window == pad->win) {
		viewmon(pad_mon);
	} else {
		if (!locateclient(&ws, &c, &pos, e->xcrossing.window)) {
			warn("attempt to enter unhandled/invisible window %d",
					e->xcrossing.window);
			return;
		}
		if (locatemon(&mon, NULL, ws) && mon != selmon) {
			viewmon(mon);
		}
		push(selmon->selws, c);
	}
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

	/* WSM boxes */
	for (i = 0; i < nws; i++) {
		if (e->xexpose.window == workspaces[i]->wsmbox) {
			renderwsmbox(workspaces[i]);
		}
	}
	if (e->xexpose.window == wsm.target->wsmbox) {
		renderwsmbox(wsm.target);
	}
}

/* fix for VTE terminals that tend to steal the focus */
void
focusin(XEvent *e)
{
	Window win = e->xfocus.window;

	if (win == root) {
		return;
	}
	if (!selmon->selws->nc || win != selmon->selws->selcli->win) {
		warn("focusin on unfocused window %d (focus is on %d)",
				e->xfocus.window,
				selmon->selws->nc ? selmon->selws->selcli->win : -1);
		updatefocus();
	}
}

int
gettiled(Client ***tiled, Monitor *mon)
{
	unsigned int i, n;

	*tiled = calloc(mon->selws->nc, sizeof(Client *));
	if (!*tiled) {
		die("could not allocate %u bytes for client list",
				mon->selws->nc*sizeof(Client *));
	}
	for (n = i = 0; i < mon->selws->nc; i++) {
		if (!mon->selws->clients[i]->floating) {
			(*tiled)[n++] = mon->selws->clients[i];
		}
	}
	*tiled = realloc(*tiled, n*sizeof(Client *));
	if (!tiled && n) {
		die("could not allocate %u bytes for client list", n*sizeof(Client *));
	}
	return n;
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
	if (wsm.active) {
		for (i = 0; i < LENGTH(wsmkeys); i++) {
			XGrabKey(dpy, XKeysymToKeycode(dpy, wsmkeys[i].key), wsmkeys[i].mod,
					root, true, GrabModeAsync, GrabModeAsync);
		}
	} else {
		for (i = 0; i < LENGTH(keys); i++) {
			XGrabKey(dpy, XKeysymToKeycode(dpy, keys[i].key), keys[i].mod, root,
					true, GrabModeAsync, GrabModeAsync);
		}
	}
}

void
hideclient(Client *c)
{
	moveresize(selmon, c, -c->w-2*BORDERWIDTH, c->y, c->w, c->h);
}

void
hidews(Workspace *ws)
{
	unsigned int i;
	for (i = 0; i < ws->nc; i++) {
		hideclient(ws->clients[i]);
	}
}

void
initbar(Monitor *mon)
{
	XSetWindowAttributes wa;

	mon->bh = dc.font.height + 2;
	mon->bar.buffer[0] = 0;

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

	/* create status bar */
	initbar(mon);

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
	ws->wsmbox = 0;
	ws->ilayout = 0;
	renamews(ws, NULL);
	return ws;
}

Window
initwsmbox(void)
{
	Window box = XCreateWindow(dpy, root, -wsm.w, -wsm.h,
			wsm.w-2*WSMBORDERWIDTH, wsm.h-2*WSMBORDERWIDTH, 0,
			DefaultDepth(dpy, screen), CopyFromParent,
			DefaultVisual(dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wsm.wa);
	XMapWindow(dpy, box);
	return box;
}

void
keypress(XEvent *e)
{
	//debug("keypress()");

	unsigned int i;
	KeySym keysym = XLookupKeysym(&e->xkey, 0);

	/* catch WSM keys if active */
	if (wsm.active) {
		for (i = 0; i < LENGTH(wsmkeys); i++) {
			if (e->xkey.state == wsmkeys[i].mod && keysym == wsmkeys[i].key &&
					wsmkeys[i].func) {
				wsmkeys[i].func(&wsmkeys[i].arg);
				updatewsm();
				return;
			}
		}
	}

	/* catch normal keys */
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
	/* TODO */
}

void
killclient(Arg const *arg)
{
	int n;
	Client *c;
	Atom protocol, request;
	Atom *supported;
	bool match=false;
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

	if (!ws) {
		return false;
	}
	for (i = 0; i < nmon; i++) {
		if (monitors[i]->selws == ws) {
			if (mon) *mon = monitors[i];
			if (pos) *pos = i;
			return true;;
		}
	}
	return false;
}

Monitor *
locaterect(int x, int y, int w, int h)
{
	Monitor *mon = selmon;
	unsigned int i;
	int a, area=0;

	for (i = 0; i < nmon; i++) {
		if ((a = INTERSECT(monitors[i], x, y, w, h)) > area) {
			area = a;
			mon = monitors[i];
		}
	}
	return mon;
}

bool
locatews(Workspace **ws, unsigned int *pos, int x, int y, char const *name)
{
	unsigned int i;

	if (name) {
		for (i = 0; i < nws; i++) {
			if (!strncmp(name, workspaces[i]->name, strlen(name))) {
				if (ws) *ws = workspaces[i];
				if (pos) *pos = i;
				return true;
			}
		}
	} else {
		for (i = 0; i < nws; i++) {
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
		XMapWindow(dpy, c->win);
		if (c->floating) {
			XRaiseWindow(dpy, c->win);
		} else {
			XLowerWindow(dpy, c->win);
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
	Monitor *mon;
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
				moveresize(selmon, c, cx, cy, c->w, c->h);
				break;
		}
	} while (ev.type != ButtonRelease);

	/* check if it has been moved to another monitor */
	mon = locaterect(selmon->x+c->x, selmon->y+c->y, c->w, c->h);
	if (mon != selmon) {
		detachclient(c);
		attachclient(mon->selws, c);
		viewmon(mon);
		updatefocus();
	}

	XUngrabPointer(dpy, CurrentTime);
}

void
moveresize(Monitor *mon, Client *c, int x, int y, int w, int h)
{
	if (checksizehints(c, x, y, &w, &h)) {
		XMoveResizeWindow(dpy, c->win, mon->x + (c->x=x), mon->y + (c->y=y),
				c->w = MAX(w, 1), c->h = MAX(h, 1));
	}
}

void
movews(Arg const *arg)
{
	Workspace *src=NULL, *dst=NULL;
	Workspace *ref = wsm.active ? wsm.target : selmon->selws;
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
	if (wsm.active) {
		wsm.target->x = dx;
		wsm.target->y = dy;
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
	debug("quit()");

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
	if (name && strlen(name) && name[0] != '*' &&
			!locatews(NULL, NULL, 0, 0, name)) {
		strncpy(ws->name, name, 256);
	} else {
		sprintf(ws->name, "*%p", (void *) ws);
	}
}

void
renderbar(Monitor *mon)
{
	/* TODO X pixel offset */

	/* background */
	XSetForeground(dpy, dc.gc, CBGNORM);
	XFillRectangle(dpy, mon->bar.win, dc.gc, 0, 0, mon->bw, mon->bh);

	/* foreground */
	XSetForeground(dpy, dc.gc, mon == selmon ? CBORDERSEL : CNORM);
	Xutf8DrawString(dpy, mon->bar.win, dc.font.xfontset, dc.gc, 6,
			dc.font.ascent+1, mon->bar.buffer, strlen(mon->bar.buffer));
	XSync(dpy, false);
}

void
renderwsmbox(Workspace *ws)
{
	/* border */
	XSetWindowBorderWidth(dpy, ws->wsmbox, WSMBORDERWIDTH);
	XSetWindowBorder(dpy, ws->wsmbox, ws == selmon->selws ? WSMCBORDERSEL
			: ws == wsm.target ? WSMCBORDERTARGET : WSMCBORDERNORM);

	/* background */
	XSetForeground(dpy, dc.gc, ws == selmon->selws ? WSMCBGSEL
			: ws == wsm.target ? WSMCBGTARGET : WSMCBGNORM);
	XFillRectangle(dpy, ws->wsmbox, dc.gc, 0, 0, wsm.w, wsm.h);

	/* text */
	XSetForeground(dpy, dc.gc, ws == selmon->selws ? WSMCSEL
			: ws == wsm.target ? WSMCTARGET : WSMCNORM);
	Xutf8DrawString(dpy, ws->wsmbox, dc.font.xfontset, dc.gc, 2,
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

	/* set pointer position to lower right */
	XWarpPointer(dpy, None, c->win, 0, 0, 0, 0,
			c->w+2*BORDERWIDTH-1, c->h+2*BORDERWIDTH-1);
	x = selmon->x+c->x+c->w+2*BORDERWIDTH-1;
	y = selmon->y+c->y+c->h+2*BORDERWIDTH-1;

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
				moveresize(selmon, c, c->x, c->y, cw, ch);
				break;
		}
	} while (ev.type != ButtonRelease);

	XUngrabPointer(dpy, CurrentTime);
}

void
restart(Arg const *arg)
{
	debug("restart()");

	restarting = true;
	running = false;
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
	while (running && !XNextEvent(dpy, &e)) {
		//debug("run(): e.type=%d", e.type);
		if (handle[e.type]) {
			handle[e.type](&e);
		}
		if (dmenu_state != DMENU_INACTIVE) {
			FD_ZERO(&fds);
			FD_SET(dmenu_out, &fds);
			ret = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
			if (ret > 0) {
				dmenueval();
			}
		}
	}
}

void
savesession(Arg const *arg)
{
	FILE *f;
	char fname[256];
	unsigned int i, j;
	Client *c;
	Workspace *ws;

	/* TODO check if file exists already (unlikely but possible) */
	srand(time(NULL));
	snprintf(fname, 256, "/tmp/%s_session_%d", APPNAME, rand());
	f = fopen(fname, "w");

	/* save monitors */
	fprintf(f, "%u\n", nmon);
	for (i = 0; i < nmon; i++) {
		fprintf(f, "%d:%d\n", monitors[i]->selws->x, monitors[i]->selws->y);
	}

	/* save workspaces and their clients */
	fprintf(f, "%u\n", nws);
	for (i = 0; i < nws; i++) {
		ws = workspaces[i];
		fprintf(f, "%d:%u:%f:%d:%d:%d:%s\n",
				ws->nmaster, ws->nc, ws->mfact, ws->x, ws->y, ws->ilayout, ws->name);
		for (j = 0; j < workspaces[i]->nc; j++) {
			c = workspaces[i]->clients[j];
			fprintf(f, "  %lu:%d:%d:%u:%u:%d:%d:%d:%d:%d\n",
					c->win, c->x, c->y, c->w, c->h, c->floating,
					c->basew, c->baseh, c->incw, c->inch);
		}
	}

	fclose(f);
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
setclientmask(Monitor *mon, bool set)
{
	int i;

	if (!mon) {
		return;
	}
	if (set) {
		for (i = 0; i < mon->selws->nc; i++) {
			XSelectInput(dpy, mon->selws->clients[i]->win, CLIENTMASK);
		}
	} else {
		for (i = 0; i < mon->selws->nc; i++) {
			XSelectInput(dpy, mon->selws->clients[i]->win, 0);
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
setpad(Arg const *arg)
{
	Client *newpad=NULL;

	/* if pad has focus, unpad it */
	if (pad_mon == selmon) {
		pad->floating = false;
		attachclient(selmon->selws, pad);
		pad = NULL;
		pad_mon = NULL;
		updatefocus();
		return;
	}

	/* pad does not have focus; check if anything has focus */
	if (!selmon->selws->nc) {
		return;
	}

	/* set current focus as pad, remove old pad if necessary */
	newpad = selmon->selws->selcli;
	detachclient(newpad);
	if (pad) {
		pad->floating = false;
		attachclient(selmon->selws, pad);
	}
	pad = newpad;
	togglepad(NULL);
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

	/* events */
	wa.event_mask = SubstructureNotifyMask|SubstructureRedirectMask|
			KeyPressMask|StructureNotifyMask|ButtonPressMask|PointerMotionMask|
			FocusChangeMask;
	XChangeWindowAttributes(dpy, root, CWEventMask, &wa);

	/* input: mouse, keyboard */
	cursor[CURSOR_NORMAL] = XCreateFontCursor(dpy, XC_left_ptr);
	cursor[CURSOR_RESIZE] = XCreateFontCursor(dpy, XC_sizing);
	cursor[CURSOR_MOVE] = XCreateFontCursor(dpy, XC_fleur);
	wa.cursor = cursor[CURSOR_NORMAL];
	XChangeWindowAttributes(dpy, root, CWCursor, &wa);
	grabkeys();

	/* high: layouts (needed here, for updategeom() below) */
	for (nlayouts=0; nlayouts<LENGTH(layouts) && layouts[nlayouts].icon; nlayouts++);

	/* output: font, monitors */
	setupfont();
	updategeom();

	/* high: workspace map, scratchpad */
	setupwsm();
	pad = NULL;
	pad_mon = NULL;
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
setupwsm(void)
{
	/* WSM data */
	wsm.rad = WSMRADIUS;
	wsm.rows = 2*wsm.rad+1;
	wsm.cols = 2*wsm.rad+1;
	wsm.active = false;

	/* WSM window informations */
	wsm.wa.override_redirect = true;
	wsm.wa.background_pixmap = ParentRelative;
	wsm.wa.event_mask = ExposureMask;

	/* dummy workspace for target box */
	wsm.target = initws(0, 0);
	wsm.target->name[0] = 0;

	/* bar input */
	wsm.im = XOpenIM(dpy, NULL, NULL, NULL);
	if (!wsm.im) {
		die("could not open input method");
	}
	wsm.ic = XCreateIC(wsm.im, XNInputStyle, XIMPreeditNothing|XIMStatusNothing,
			XNClientWindow, root, NULL);
	if (!wsm.ic) {
		die("could not open input context");
	}
	XSetICFocus(wsm.ic);
}

void
setws(Monitor *mon, int x, int y)
{
	Workspace *next=NULL;
	Monitor *othermon=NULL;

	if (locatews(&next, NULL, x, y, NULL) && mon->selws == next) {
		return;
	}

	/* exchange monitors on collision */
	if (locatemon(&othermon, NULL, next) && othermon != mon) {
		othermon->selws = mon->selws;
		mon->selws = next;
		arrange(mon);
		arrange(othermon);
		updatebar(mon);
		updatebar(othermon);
		updatefocus();
		return;
	}

	/* current workspace */
	if (mon->selws->nc) {
		hidews(mon->selws);
	} else {
		detachws(mon->selws);
		termws(mon->selws);
	}

	/* next workspace */
	if (next) {
		mon->selws = next;
	} else {
		mon->selws = initws(x, y);
		attachws(mon->selws);
	}

	arrange(mon);
	updatebar(mon);
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
	if (selmon->selws->selcli->floating) {
		XRaiseWindow(dpy, selmon->selws->selcli->win);
	}
}

void
steplayout(Arg const *arg)
{
	selmon->selws->ilayout = (selmon->selws->ilayout+nlayouts+arg->i)%nlayouts;
	arrange(selmon);
}

void
stepmon(Arg const *arg)
{
	Monitor *oldmon;
	unsigned int pos;

	if (!locatemon(&oldmon, &pos, selmon->selws)) {
		warn("attempt to step from non-existing monitor");
		return;
	}
	oldmon = selmon;
	viewmon(monitors[(pos+nmon+1)%nmon]);
	updatefocus();
}

void
stepws(Arg const *arg)
{
	int x, y;
	reltoxy(&x, &y, selmon->selws, arg->i);
	setws(selmon, x, y);
}

void
stepwsmbox(Arg const *arg)
{
	Workspace *ws;
	reltoxy(&wsm.target->x, &wsm.target->y, wsm.target, arg->i);
	if (locatews(&ws, NULL, wsm.target->x, wsm.target->y, NULL)) {
		strcpy(wsm.target->name, ws->name);
	} else {
		wsm.target->name[0] = 0;
	}
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
togglepad(Arg const *arg)
{
	if (!pad) {
		return;
	}
	setclientmask(selmon, false);
	if (pad_mon == selmon) {
		hideclient(pad);
		pad_mon = NULL;
	} else {
		pad->floating = true;
		pad_mon = selmon;
		moveresize(selmon, pad,
				selmon->wx + PADMARGIN,
				selmon->wy + PADMARGIN,
				selmon->ww - 2*PADMARGIN - 2*BORDERWIDTH,
				selmon->wh - 2*PADMARGIN - 2*BORDERWIDTH);
	}
	updatefocus();
	setclientmask(selmon, true);
}

void
togglewsm(Arg const *arg)
{
	unsigned int i;

	wsm.active = !wsm.active;

	if (!wsm.active) {
		for (i = 0; i < nws; i++) {
			if (workspaces[i]->wsmbox) {
				XDestroyWindow(dpy, workspaces[i]->wsmbox);
				workspaces[i]->wsmbox = 0;
			}
		}
		XDestroyWindow(dpy, wsm.target->wsmbox);
		wsm.target->wsmbox = 0;
		updatebar(selmon);
		grabkeys();
		return;
	}

	/* create a box for each workspace and hide it */
	wsm.w = selmon->ww/wsm.cols;
	wsm.h = selmon->h/wsm.rows;
	for (i = 0; i < nws; i++) {
		workspaces[i]->wsmbox = initwsmbox();
	}
	wsm.target->wsmbox = initwsmbox();

	/* initial target is the current workspace (make a "0-step") */
	wsm.target->x = selmon->selws->x;
	wsm.target->y = selmon->selws->y;
	stepwsmbox(&((Arg const) { .i = NO_DIRECTION }));

	/* grab WSM keys and give an initial update */
	grabkeys();
	updatewsm();
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

	if (pad && e->xunmap.window == pad->win) {
		termclient(pad);
		if (pad_mon) {
			pad_mon = NULL;
			updatefocus();
		}
		pad_mon = NULL;
		pad = NULL;
	} else if (locateclient(&ws, &c, NULL, e->xunmap.window)) {
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
	sprintf(mon->bar.buffer, "%s  %s", layouts[mon->selws->ilayout].icon,
				mon->selws->name);
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
		sel = mon == selmon && pad_mon != selmon;

		/* empty monitor: don't focus anything */
		if (!mon->selws->ns) {
			if (sel) {
				XSetInputFocus(dpy, root, RevertToPointerRoot, CurrentTime);
			}
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

	/* focus scratchpad if existent and visible */
	if (pad) {
		if (pad_mon == selmon) {
			XSetInputFocus(dpy, pad->win, RevertToPointerRoot, CurrentTime);
			XSetWindowBorder(dpy, pad->win, CBORDERSEL);
			XRaiseWindow(dpy, pad->win);
			grabbuttons(pad, true);
		} else {
			XSetWindowBorder(dpy, pad->win, CBORDERNORM);
			grabbuttons(pad, false);
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

	mon->wx = 0;
	mon->wy = mon->bh;
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
updatewsm(void)
{
	unsigned int i;

	if (!wsm.active) {
		return;
	}

	for (i = 0; i < nws; i++) {
		updatewsmbox(workspaces[i]);
	}
	updatewsmbox(wsm.target);
	XSync(dpy, false);
}

void
updatewsmbox(Workspace *ws)
{
	int c, r, x, y;

	/* hide box if not in range */
	if (ws->x < wsm.target->x-wsm.rad || ws->x > wsm.target->x+wsm.rad ||
			ws->y < wsm.target->y-wsm.rad || ws->y > wsm.target->y+wsm.rad) {
		XMoveWindow(dpy, ws->wsmbox, -wsm.w, -wsm.h);
		return;
	}

	/* show box */
	c = ws->x + wsm.rad - wsm.target->x;
	r = ws->y + wsm.rad - wsm.target->y;
	x = selmon->x + selmon->wx + (selmon->ww/2 - wsm.w*wsm.rad - wsm.w/2 + c*wsm.w);
	y = selmon->y + selmon->wy + (selmon->wh/2 - wsm.h*wsm.rad - wsm.h/2 + r*wsm.h);
	XMoveWindow(dpy, ws->wsmbox, x, y);
	XRaiseWindow(dpy, ws->wsmbox);

	renderwsmbox(ws);
}

void
viewmon(Monitor *mon)
{
	Monitor *oldmon = selmon;
	selmon = mon;
	updatebar(oldmon);
	updatebar(selmon);
}

void
viewws(Arg const *arg)
{
	if (!wsm.active) {
		return;
	}
	setws(selmon, wsm.target->x, wsm.target->y);
	togglewsm(NULL);
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

/* layouts */

void
bstack(Monitor *mon)
{
	Client **tiled;
	unsigned int i, ncm, nct, w, h;
	int y;

	/* get tiled clients */
	if (!mon->selws->nc || !(nct = gettiled(&tiled, mon))) {
		return;
	}

	/* draw master area */
	ncm = MIN(mon->selws->nmaster, nct);
	if (ncm) {
		y = mon->wy;
		w = mon->ww/ncm;
		h = ncm == nct ? mon->wh : mon->selws->mfact*mon->wh;
		for (i = 0; i < ncm; i++) {
			moveresize(mon, tiled[i], mon->wx+i*w, y,
					w-2*BORDERWIDTH, h-2*BORDERWIDTH);
		}
	}
	if (ncm == nct) {
		free(tiled);
		return;
	}

	/* draw stack area */
	y = mon->wy+(ncm ? mon->selws->mfact*mon->wh : mon->wy);
	w = mon->ww/(nct-ncm);
	h = ncm ? mon->h-y : mon->wh;
	for (i = ncm; i < nct; i++) {
		moveresize(mon, tiled[i], mon->wx+(i-ncm)*w, y,
				w-2*BORDERWIDTH, h-2*BORDERWIDTH);
	}
	free(tiled);
}

void
rstack(Monitor *mon)
{
	Client **tiled;
	unsigned int i, ncm, nct, w, h;
	int x;

	/* get tiled clients */
	if (!mon->selws->nc || !(nct = gettiled(&tiled, mon))) {
		return;
	}

	/* draw master area */
	ncm = MIN(mon->selws->nmaster, nct);
	if (ncm) {
		x = mon->wx;
		w = ncm == nct ? mon->ww : mon->selws->mfact*mon->ww;
		h = mon->wh/ncm;
		for (i = 0; i < ncm; i++) {
			moveresize(mon, tiled[i], x, mon->wy+i*h,
					w-2*BORDERWIDTH, h-2*BORDERWIDTH);
		}
	}
	if (ncm == nct) {
		free(tiled);
		return;
	}

	/* draw stack area */
	x = mon->wx+(ncm ? mon->selws->mfact*mon->ww : mon->wx);
	w = ncm ? mon->w-x : mon->ww;
	h = mon->wh/(nct-ncm);
	for (i = ncm; i < nct; i++) {
		moveresize(mon, tiled[i], x, mon->wy+(i-ncm)*h,
				w-2*BORDERWIDTH, h-2*BORDERWIDTH);
	}
	free(tiled);
}

/* main */

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

