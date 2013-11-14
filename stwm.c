#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/Xproto.h>

/* macros */
#define DEBUG 0 /* enable for debug output */
#define debug(...) if (DEBUG) stdlog(stdout, "\033[34mDBG\033[0m "__VA_ARGS__)
#define warn(...) stdlog(stderr, "\033[33mWRN\033[0m "__VA_ARGS__)
#define die(...) warn("\033[31mERR\033[0m "__VA_ARGS__); exit(EXIT_FAILURE)
#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))

typedef union {
	int i;
	float f;
	const void *v;
} Arg;

typedef struct Client Client;
struct Client {
	int x,y,w,h;
	char name[256];
	Window win;
	bool floating;
};

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
} Workspace;

/* functions */
static void arrange(void);
static void cleanup(void);
static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void clientmessage(XEvent *);
static void configurerequest(XEvent *);
static void configurenotify(XEvent *);
static void createnotify(XEvent *);
static void destroynotify(XEvent *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void focusin(XEvent *);
static void focusstep(Arg const *);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void manage(Window);
static void mapnotify(XEvent *);
static void mappingnotify(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void pop(Client *);
static void propertynotify(XEvent *);
static void push(Client *);
static void quit(Arg const *);
static void restart(Arg const *);
static void run(void);
static void scan(void);
static void setmfact(Arg const *);
static void setup(void);
static void spawn(Arg const *);
static void stdlog(FILE *, char const *, ...);
static void tile(void);
static void unmapnotify(XEvent *);
static void updatefocus(void);
static Client *wintoclient(Window, unsigned int *);
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
static char *appname;
static Display *dpy;
static bool running, restarting;
static Window root;
static int screen;
static unsigned int sw, sh; /* screen dimensions */
static Workspace ws;

/* configuration */
#include "config.h"

void
arrange(void)
{
	unsigned int i = 0;

	/* disable EnterWindowMask, so the focus won't change at every rearrange */
	for (i = 0; i < ws.nc; i++) {
		XSelectInput(dpy, ws.clients[i]->win, 0);
	}
	tile();
	for (i = 0; i < ws.nc; i++) {
		XSelectInput(dpy, ws.clients[i]->win, EnterWindowMask);
	}
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
	for (i = 0; i < ws.nc; i++) {
		free(ws.clients[i]);
	}
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
	//debug("configurenotify(%d)", e->xconfigure.window);

	XWindowAttributes wa;
	Client *c = wintoclient(e->xconfigure.window, NULL);

	/* check if window is managed */
	if (!c) {
		return;
	}

	/* correct the size of the window if necessary (size hints) */
	if (!XGetWindowAttributes(dpy, c->win, &wa)) {
		warn("XGetWindowAttributes(%d) failed", c->win);
		return;
	}
	if (wa.x != c->x || wa.y != c->y || wa.width != c->w || wa.height != c->h) {
		XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w-2*borderwidth,
				c->h-2*borderwidth);
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
	debug("\033[32mcreatenotify(%d)\033[0m", e->xcreatewindow.window);
	/* TODO */
}

void
destroynotify(XEvent *e)
{
	debug("\033[31mdestroynotify(%d)\033[0m", e->xdestroywindow.window);
	/* TODO */
}

void
enternotify(XEvent *e)
{
	debug("enternotify(%d)", e->xcrossing.window);

	Client *c = wintoclient(e->xcrossing.window, NULL);
	if (!c) {
		warn("attempt to enter unhandled window %d", e->xcrossing.window);
		return;
	}

	push(c);
	updatefocus();
}

void
expose(XEvent *e)
{
	debug("expose(%d)", e->xexpose.window);
	/* TODO */
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

	if (!ws.nc) {
		return;
	}
	wintoclient(ws.selcli->win, &pos);
	push(ws.clients[(pos+ws.nc+arg->i)%ws.nc]);
	updatefocus();
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
keypress(XEvent *e)
{
	//debug("keypress()");

	unsigned int i;
	KeySym keysym = XLookupKeysym(&e->xkey, 0);

	for (i = 0; i < LENGTH(keys); i++) {
		if (keys[i].mod == e->xkey.state && keys[i].key == keysym &&
				keys[i].func) {
			keys[i].func(&keys[i].arg);
		}
	}
}

void
keyrelease(XEvent *e)
{
	//debug("keyrelease(%d)", e->xkey.window);
	/* TODO */
}

void
manage(Window w)
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

	/* add to list */
	ws.clients = realloc(ws.clients, ++ws.nc*sizeof(Client *));
	if (!ws.clients) {
		die("could not allocate %d bytes for client list",
				ws.nc*sizeof(Client *));
	}
	ws.clients[ws.nc-1] = c;

	/* configure */
	c->win = w;
	XSelectInput(dpy, c->win, EnterWindowMask);
	XSetWindowBorderWidth(dpy, c->win, borderwidth);

	/* update layout */
	arrange();
	push(c);
	updatefocus();
}

void
mapnotify(XEvent *e)
{
	debug("\033[1;32mmapnotify(%d)\033[0m", e->xmap.window);

	manage(e->xmap.window);
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
pop(Client *c)
{
	unsigned int i;

	if (!c) {
		warn("attempt to pop NULL client");
		return;
	}

	for (i = 0; i < ws.ns; i++) {
		if (ws.stack[i] == c) {
			ws.ns--;
			for (; i < ws.ns; i++) {
				ws.stack[i] = ws.stack[i+1];
			}
			ws.stack = realloc(ws.stack, ws.ns*sizeof(Client *));
			if (ws.ns && !ws.stack) {
				die("could not allocate %d bytes for stack",
						ws.ns*sizeof(Client*));
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
push(Client *c)
{
	if (!c) {
		warn("attempt to push NULL client");
	}

	pop(c);
	ws.stack = realloc(ws.stack, ++ws.ns*sizeof(Client *));
	if (!ws.stack) {
		die("could not allocated %d bytes for stack", ws.ns*sizeof(Client *));
	}
	ws.stack[ws.ns-1] = c;
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
		//debug("\033[1;30mrun(): e.type=%d\033[0m", e.type);
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
		manage(wins[i]); /* manage() will do the necessary checks */
	}
}

void
setmfact(Arg const *arg)
{
	ws.mfact = MAX(0.1, MIN(0.9, ws.mfact+arg->f));
	arrange();
}

void
setup(void)
{
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);
	XSelectInput(dpy, root, SubstructureNotifyMask|KeyPressMask);
	xerrorxlib = XSetErrorHandler(xerror);
	grabkeys();
	ws.nc = 0;
	ws.nmaster = nmaster;
	ws.mfact = mfact;
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
	debug("\033[34mtile()\033[0m");

	int ncm, i, x, w, h;

	if (!ws.nc) {
		return;
	}

	/* draw master area */
	ncm = MIN(ws.nmaster, ws.nc);
	x = 0;
	w = ws.nmaster >= ws.nc ? sw : ws.mfact*sw;
	h = sh/ncm;
	for (i = 0; i < ncm; i++) {
		ws.clients[i]->x = x;
		ws.clients[i]->y = i*h;
		ws.clients[i]->w = w;
		ws.clients[i]->h = h;
		XMoveResizeWindow(dpy, ws.clients[i]->win,
				ws.clients[i]->x, ws.clients[i]->y,
				ws.clients[i]->w-2*borderwidth, ws.clients[i]->h-2*borderwidth);
	}
	if (ncm == ws.nc) {
		return;
	}

	/* draw stack area */
	x = ws.mfact*sw;
	w = sw-x;
	h = sh/(ws.nc-ncm);
	for (; i < ws.nc; i++) {
		ws.clients[i]->x = x;
		ws.clients[i]->y = (i-ncm)*h;
		ws.clients[i]->w = w;
		ws.clients[i]->h = h;
		XMoveResizeWindow(dpy, ws.clients[i]->win,
				ws.clients[i]->x, ws.clients[i]->y,
				ws.clients[i]->w-2*borderwidth, ws.clients[i]->h-2*borderwidth);
	}
}

void
unmapnotify(XEvent *e)
{
	debug("\033[1;31munmapnotify(%d)\033[0m", e->xunmap.window);

	unsigned int i;
	Client *c = wintoclient(e->xunmap.window, &i);
	if (!c) {
		return;
	}

	/* remove from stack */
	pop(c);

	/* remove client */
	free(c);
	ws.nc--;
	for (; i < ws.nc; i++) {
		ws.clients[i] = ws.clients[i+1];
	}

	/* update layout */
	arrange();
	updatefocus();
}

void
updatefocus(void)
{
	unsigned int i;

	if (!ws.ns) {
		return;
	}

	/* unfocus all but the top of the stack */
	for (i = 0; i < ws.ns-1; i++) {
		XSetWindowBorder(dpy, ws.stack[i]->win, cbordernorm);
	}

	/* focus top of the stack */
	ws.selcli = ws.stack[ws.ns-1];
	XSetWindowBorder(dpy, ws.selcli->win, cbordersel);
	XSetInputFocus(dpy, ws.selcli->win, RevertToPointerRoot, CurrentTime);
}

Client *
wintoclient(Window w, unsigned int *pos)
{
	unsigned int i;
	for (i = 0; i < ws.nc; i++) {
		if (ws.clients[i]->win == w) {
			if (pos) {
				*pos = i;
			}
			return ws.clients[i];
		}
	}
	return NULL;
}

int
xerror(Display *dpy, XErrorEvent *ee)
{
	char es[256];

	/* only display error on this error instead of crashing */
	if (
		(ee->error_code == BadWindow) ||
		(ee->request_code == X_ChangeWindowAttributes && ee->error_code == BadMatch) ||
		(ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch) ||
		(ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	) {
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

	if (!ws.nc) {
		return;
	}

	c = wintoclient(ws.selcli->win, &pos);
	if (!c) {
		warn("attempt to zoom non-existing window %d", ws.selcli->win);
		return;
	}

	if (!pos) {
		/* window is at the top */
		if (ws.nc > 1) {
			ws.clients[0] = ws.clients[1];
			ws.clients[1] = c;
			push(ws.clients[0]);
			updatefocus();
		} else {
			return;
		}
	} else {
		/* window is somewhere else */
		for (i = pos; i > 0; i--) {
			ws.clients[i] = ws.clients[i-1];
		}
		ws.clients[0] = c;
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
	setup();
	scan();
	run();
	cleanup();
	XCloseDisplay(dpy);
	stdlog(stdout, "shutting down ...");
	if (restarting) {
		execl("stwm", appname, NULL); /* TODO change to execlp */
	}
	return EXIT_SUCCESS;
}

