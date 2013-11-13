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
#define DEBUG 1 /* enable for debug output */
#define debug(...) if (DEBUG) stdlog(stdout, "\033[34mDBG\033[0m "__VA_ARGS__)
#define warn(...) stdlog(stderr, "\033[33mWRN\033[0m "__VA_ARGS__)
#define die(...) warn("\033[31mERR\033[0m "__VA_ARGS__); exit(EXIT_FAILURE)
#define HANDLED(C) (!C->override && C->mapped)
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
	bool floating, override, mapped;
};

typedef struct {
	unsigned int mod;
	KeySym key;
	void (*func)(Arg const *);
	Arg const arg;
} Key;

/* functions */
static void arrange(void);
static void cleanup(void);
static void buttonpress(XEvent *);
static void buttonrelease(XEvent *);
static void clientmessage(XEvent *);
static void configurerequest(XEvent *);
static void configurenotify(XEvent *);
static void create(Window);
static void createnotify(XEvent *);
static void destroynotify(XEvent *);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void focus(Client *);
static void focusin(XEvent *);
static void focusstep(Arg const *);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void mapnotify(XEvent *);
static void mappingnotify(XEvent *);
static void maprequest(XEvent *);
static void motionnotify(XEvent *);
static void propertynotify(XEvent *);
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
static Client *wintoclient(Window, unsigned int *);
static int xerror(Display *, XErrorEvent *);
static int (*xerrorxlib)(Display *, XErrorEvent *);

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
static Client **clients;
static unsigned int nc, sel;

/* configuration */
#include "config.h"

void
arrange(void)
{
	unsigned int i = 0;
	for (i = 0; i < nc; i++) {
		if (clients[i]->mapped) {
			XSelectInput(dpy, clients[i]->win, 0);
		}
	}
	tile();
	for (i = 0; i < nc; i++) {
		if (clients[i]->mapped) {
			XSelectInput(dpy, clients[i]->win, EnterWindowMask);
		}
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
	for (i = 0; i < nc; i++) {
		free(clients[i]);
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
	if (!c) {
		warn("attempt to configure non-existing window %d", e->xconfigure.window);
		return;
	}

	if (!HANDLED(c)) {
		return;
	}

	if (!XGetWindowAttributes(dpy, c->win, &wa)) {
		warn("XGetWindowAttributes(%d) failed", c->win);
		return;
	}

	/* correct the size of the window if necessary (size hints) */
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
create(Window w)
{
	Client *c;
	XWindowAttributes wa;

	/* create client */
	c = malloc(sizeof(Client));
	if (!c) {
		die("could not allocate new client (%d bytes)", sizeof(Client));
	}

	/* add to list */
	clients = realloc(clients, ++nc*sizeof(Client *));
	if (!clients) {
		die("could not allocate new entry in client list (%d bytes)",
				sizeof(Client *));
	}
	clients[nc-1] = c;

	/* set client data */
	c->win = w;
	if (!XGetWindowAttributes(dpy, w, &wa)) {
		warn("XGetWindowAttributes() failed for window %d", w);
		return;
	}
	c->mapped = wa.map_state == IsViewable;
	c->override = wa.override_redirect;

	/* update screen, if necessary (= if client is already mapped) */
	if (HANDLED(c)) {
		focus(c);
		arrange();
	}
}

void
createnotify(XEvent *e)
{
	debug("\033[32mcreatenotify(%d)\033[0m", e->xcreatewindow.window);

	create(e->xcreatewindow.window);
}

void
destroynotify(XEvent *e)
{
	debug("\033[31mdestroynotify(%d)\033[0m", e->xdestroywindow.window);

	unsigned int i;
	for (i = 0; i < nc; i++) {
		if (clients[i]->win == e->xdestroywindow.window) {
			if (clients[i]->mapped) {
				warn("destroying mapped window %d", clients[i]->win);
			}
			free(clients[i]);
			nc--;
			sel -= sel > i;
			for (; i < nc; i++) {
				clients[i] = clients[i+1];
			}
			clients = realloc(clients, nc*sizeof(Client *));
			return;
		}
	}
	warn("attempt to destroy non-existing window");
}

void
enternotify(XEvent *e)
{
	debug("enternotify(%d)", e->xcrossing.window);

	focus(wintoclient(e->xcrossing.window, NULL));
}

void
expose(XEvent *e)
{
	debug("expose(%d)", e->xexpose.window);
	/* TODO */
}

void
focus(Client *c)
{
	/*
	unsigned int i;
	debug("focus(%d) with sel=%d", c->win, sel);
	for (i = 0; i < nc; i++) {
		debug("  %sclients[%d]->win = %d\033[0m%s",
				clients[i]->mapped ? "" : "\033[1;30m" , i, clients[i]->win,
				sel == i ? " <old" : clients[i]->win == c->win ? " <new" : "");
	}
	*/
	if (!c->mapped) {
		warn("attempt to focus unmapped window %d", c->win);
		return;
	}
	if (HANDLED(c)) {
		if (clients[sel]->mapped) {
			XSetWindowBorder(dpy, clients[sel]->win, cbordernorm);
		}
		wintoclient(c->win, &sel);
		XSetWindowBorder(dpy, c->win, cbordersel);
	}
	XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
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
	unsigned int i;

	if (!nc) {
		return;
	}
	if (arg->i > 0) {
		for (i=(sel+1)%nc; !clients[i]->mapped && i != sel; i=(i+1)%nc);
	} else {
		for (i=(sel-1+nc)%nc; !clients[i]->mapped && i != sel; i=(i-1+nc)%nc);
	}
	focus(clients[i]);
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
mapnotify(XEvent *e)
{
	debug("\033[1;32mmapnotify(%d)\033[0m", e->xmap.window);

	Client *c;

	c = wintoclient(e->xmap.window, NULL);
	if (!c) {
		warn("trying to map non-existing window %d", e->xmap.window);
		return;
	}
	c->mapped = true;
	c->override = e->xmap.override_redirect;
	if (HANDLED(c)) {
		XSetWindowBorderWidth(dpy, c->win, borderwidth);
		arrange();
	}
	XSelectInput(dpy, c->win, EnterWindowMask);
	focus(c);
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
propertynotify(XEvent *e)
{
	debug("propertynotify(%d)", e->xproperty.window);
	/* TODO */
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
		create(wins[i]);
	}
}

void
setmfact(Arg const *arg)
{
	mfact += arg->f;
	mfact = MAX(0.1, MIN(0.9, mfact));
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
	nc = 0;
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

	int nct, ncm, i, x, w, h;
	Client **tiled = calloc(nc, sizeof(Client *));
	if (!tiled) {
		die("could not allocate %d bytes for tiled list", nc*sizeof(Client *));
	}

	/* get tiled windows */
	for (i = 0, nct = 0; i < nc; i++) {
		if (HANDLED(clients[i])) {
			tiled[nct++] = clients[i];
		}
	}
	if (!nct) {
		free(tiled);
		return;
	}

	/* draw master area */
	ncm = MIN(nmaster, nct);
	x = 0;
	w = nmaster >= nct ? sw : mfact*sw;
	h = sh/ncm;
	for (i = 0; i < ncm; i++) {
		tiled[i]->x = x;
		tiled[i]->y = i*h;
		tiled[i]->w = w;
		tiled[i]->h = h;
		XMoveResizeWindow(dpy, tiled[i]->win, tiled[i]->x, tiled[i]->y,
				tiled[i]->w-2, tiled[i]->h-2);
	}
	if (ncm == nct) {
		free(tiled);
		return;
	}

	/* draw stack area */
	x = mfact*sw;
	w = sw-x;
	h = sh/(nct-ncm);
	for (; i < nct; i++) {
		tiled[i]->x = x;
		tiled[i]->y = (i-ncm)*h;
		tiled[i]->w = w;
		tiled[i]->h = h;
		XMoveResizeWindow(dpy, tiled[i]->win, tiled[i]->x, tiled[i]->y,
				tiled[i]->w-2*borderwidth, tiled[i]->h-2*borderwidth);
	}
	free(tiled);
}

void
unmapnotify(XEvent *e)
{
	debug("\033[1;31munmapnotify(%d)\033[0m", e->xunmap.window);

	unsigned int i;
	Client *c = wintoclient(e->xunmap.window, &i);

	if (!c) {
		warn("attempt to unmap non-existing window %d", e->xunmap.window);
		return;
	}
	if (!c->mapped) {
		warn("attempt to unmap unmapped window %d", e->xunmap.window);
		return;
	}
	c->mapped = false;

	/* update focus */
	if (i == sel) {
		for (i=(sel-1+nc)%nc; !clients[i]->mapped && i != sel; i=(i-1+nc)%nc);
		if (i != sel) {
			focus(clients[i]);
		}
	}
	arrange();
}

Client *
wintoclient(Window w, unsigned int *pos)
{
	unsigned int i;
	for (i = 0; i < nc; i++) {
		if (clients[i]->win == w) {
			if (pos) {
				*pos = i;
			}
			return clients[i];
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

