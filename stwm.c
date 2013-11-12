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
void cleanup(void);
void buttonpress(XEvent *);
void buttonrelease(XEvent *);
void clientmessage(XEvent *);
void configurerequest(XEvent *);
void configurenotify(XEvent *);
void create(Window);
void createnotify(XEvent *);
void destroynotify(XEvent *);
void enternotify(XEvent *);
void expose(XEvent *);
void focus(Client *);
void focusin(XEvent *);
void focusstep(Arg const *);
void keypress(XEvent *);
void keyrelease(XEvent *);
void mapnotify(XEvent *);
void mappingnotify(XEvent *);
void maprequest(XEvent *);
void motionnotify(XEvent *);
void propertynotify(XEvent *);
void quit(Arg const *);
void restart(Arg const *);
void run(void);
void scan(void);
void setmfact(Arg const *);
void setup(void);
void stdlog(FILE *, char const *, ...);
void tile(void);
void unfocus(Client *);
void unmapnotify(XEvent *);
Client *wintoclient(Window, unsigned int *);
int xerror(Display *, XErrorEvent *);
int (*xerrorxlib)(Display *, XErrorEvent *);

/* event handlers, as array to allow O(1) access; codes in X.h */
void (*handle[LASTEvent])(XEvent *) = {
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
char *appname;
Display *dpy;
bool running, restarting;
Window root;
int screen;
unsigned int sw, sh; /* screen dimensions */
Client **clients;
unsigned int nc, sel;

/* configuration */
#include "config.h"

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
	Client *c = wintoclient(e->xconfigure.window, NULL);

	c->override = e->xconfigure.override_redirect;
	if (HANDLED(c)) {
		tile();
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
		die("could not allocate new entry in client list (%d bytes)", sizeof(Client *));
	}
	clients[nc-1] = c;

	/* set client data */
	c->win = w;
	if (!XGetWindowAttributes(dpy, w, &wa)) {
		warn("XGetWindowAttributes() failed for window %d", w);
		return;
	}
	c->mapped = wa.map_state;
	c->override = wa.override_redirect;

	/* update screen, if necessary (= if client is already mapped) */
	if (HANDLED(c)) {
		sel = nc-1;
		tile();
	}
}

void
createnotify(XEvent *e)
{
	create(e->xcreatewindow.window);
}

void
destroynotify(XEvent *e)
{
	unsigned int i;
	for (i = 0; i < nc; i++) {
		if (clients[i]->win == e->xdestroywindow.window) {
			if (clients[i]->mapped) {
				warn("destroying mapped window %d", clients[i]->win);
			}
			free(clients[i]);
			nc--;
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
	debug("enternotify(?)");
	/* TODO */
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
		for (i = (sel+1)%nc; !clients[i]->mapped && i != sel; i = (i+1)%nc);
	} else {
		for (i = (sel-1+nc)%nc; !clients[i]->mapped && i != sel; i = (i-1+nc)%nc);
	}
	sel = i;
	focus(clients[sel]);
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
	debug("keyrelease(%d)", e->xkey.window);
	/* TODO */
}

void
mapnotify(XEvent *e)
{
	Client *c = wintoclient(e->xmap.window, &sel);
	if (!c) {
		warn("trying to map non-existing window %d", e->xmap.window);
		return;
	}
	c->mapped = true;
	c->override = e->xmap.override_redirect;

	focus(c);
	tile();
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
	tile();
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
	int nct, ncm, i, x, w, h;
	Client **tiled = calloc(nc, sizeof(Client *));

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
				tiled[i]->w, tiled[i]->h);
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
				tiled[i]->w, tiled[i]->h);
	}
	free(tiled);
}

void
unfocus(Client *c)
{
	/* TODO update border colour */
}

void
unmapnotify(XEvent *e)
{
	unsigned int i;
	Client *c = wintoclient(e->xunmap.window, NULL);

	if (!c) {
		warn("attempt to unmap non-existing window %d", e->xunmap.window);
		return;
	}
	if (!c->mapped) {
		warn("attempt to unmap unmapped window %d", e->xunmap.window);
		return;
	}
	c->mapped = false;
	tile();

	/* update focus */
	for (i = (sel-1+nc)%nc; !clients[i]->mapped && i != sel; i = (i-1+nc)%nc);
	sel = i;
	if (nc && clients[sel]->mapped) {
		focus(clients[sel]);
	}
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

