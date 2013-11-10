#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdarg.h>
#include <time.h>

/* macros */
#define DEBUG 1 /* enable for debug output */
#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) < (y) ? (y) : (x))
#define debug(...) stdlog(stdout, __VA_ARGS__)
#define warn(...) stdlog(stderr, __VA_ARGS__)
#define die(...) warn(__VA_ARGS__); exit(EXIT_FAILURE)
#define MODKEY Mod4Mask

/* enums */
enum { CURSOR_NORMAL, CURSOR_RESIZE, CURSOR_MOVE, CURSOR_LAST };
enum Flag {
	CLIENT_FLOATING = 1
};

/* structs */
typedef struct Client Client;
struct Client {
	int x,y,w,h;
	long flags;
	char name[256];
	Window win;
};

/* functions */
void attach(Window);
void cleanup(void);
void detach(Client *);
void focus(Client *);
void focusstep(int);
void handleButtonPress(XEvent *);
void handleClientMessage(XEvent *);
void handleConfigureRequest(XEvent *);
void handleConfigureNotify(XEvent *);
void handleCreateNotify(XEvent *);
void handleDestroyNotify(XEvent *);
void handleEnterNotify(XEvent *);
void handleExpose(XEvent *);
void handleFocusIn(XEvent *);
void handleKeyPress(XEvent *);
void handleKeyRelease(XEvent *);
void handleMapNotify(XEvent *);
void handleMappingNotify(XEvent *);
void handleMapRequest(XEvent *);
void handleMotionNotify(XEvent *);
void handlePropertyNotify(XEvent *);
void handleUnmapNotify(XEvent *);
void quit(void);
void restart(void);
void run(void);
void scan(void);
void setmfact(float);
void setup(void);
void stdlog(FILE *, char const *, ...);
void tile(void);
void unfocus(Client *);
Client *wintoclient(Window);
int xerror(Display *, XErrorEvent *);
int (*xerrorxlib)(Display *, XErrorEvent *);

/* event handlers, as array to allow O(1) access */
void (*handle[LASTEvent])(XEvent *) = {
	[ButtonPress] = handleButtonPress,
	[ClientMessage] = handleClientMessage,
	[ConfigureRequest] = handleConfigureRequest,
	[ConfigureNotify] = handleConfigureNotify,
	[CreateNotify] = handleCreateNotify,
	[DestroyNotify] = handleDestroyNotify,
	[EnterNotify] = handleEnterNotify,
	[Expose] = handleExpose,
	[FocusIn] = handleFocusIn,
	[KeyPress] = handleKeyPress,
	[KeyRelease] = handleKeyRelease,
	[MapNotify] = handleMapNotify,
	[MappingNotify] = handleMappingNotify,
	[MapRequest] = handleMapRequest,
	[MotionNotify] = handleMotionNotify,
	[PropertyNotify] = handlePropertyNotify,
	[UnmapNotify] = handleUnmapNotify
};

/* variables */
int cbg, cborder; /* TODO */
Cursor cursor[CURSOR_LAST]; /* TODO */
Display *dpy;
bool running, restarting;
Window root;
int screen;
int sw, sh; /* screen dimensions */
Client **clients;
int nc, nmaster, sel;
float mfact;

/* HANDLER FUNCTIONS -------------------------------------------------------- */

/* 4 */
void
handleButtonPress(XEvent *e)
{
	debug("ButtonPress (%d)", e->type);
}

/* 5 */
void
handleButtonRelease(XEvent *e)
{
	debug("ButtonRelease (%d)", e->type);
}

/* 33 */
void
handleClientMessage(XEvent *e)
{
	long *l = e->xclient.data.l;
	short *s = e->xclient.data.s;
	char *b = e->xclient.data.b;

	debug("ClientMessage", e->type);
	debug("  send_event=%s", e->xclient.send_event ? "true" : "false");
	debug("  window=%d", e->xclient.window);
	debug("  format=%d", e->xclient.format);
	switch (e->xclient.format) {
		case 32:
			debug("  l={%ld,%ld,%ld,%ld,%ld}",
					l[0],l[1],l[2],l[3],l[4]);
			break;
		case 16:
			debug("  s={%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd,%hd}",
					s[0],s[1],s[2],s[3],s[4],s[5],s[6],s[7],s[8],s[9]);
			break;
		case 8:
			debug("  s={%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c,%c}",
					b[0],b[1],b[2],b[3],b[4],b[5],b[6],b[7],b[8],b[9],
					b[10],b[11],b[12],b[13],b[14],b[15],b[16],b[17],b[18],b[19]);
			break;
		default:
			debug("  unknown data format: %d", e->xclient.format);
	}
}

/* 22 */
void
handleConfigureNotify(XEvent *e)
{
	debug("\033[35mconfigure window %d%s\033[0m", e->xconfigure.window,
			e->xconfigure.override_redirect ? " => with override_redirect":"");
}

/* 23 */
void
handleConfigureRequest(XEvent *e)
{
	debug("ConfigureRequest (%d)", e->type);
}

/* 16 */
void
handleCreateNotify(XEvent *e)
{
	debug("\033[32mcreate window %d on window %d (%c= root)%s\033[0m",
			e->xcreatewindow.window, e->xcreatewindow.parent,
			e->xcreatewindow.parent == root ? '=' : '!',
			e->xcreatewindow.override_redirect?" => with override_redirect":"");
}

/* 17 */
void
handleDestroyNotify(XEvent *e)
{
	debug("\033[31mdestroy window %d\033[0m", e->xdestroywindow.window);
}

/* 7 */
void
handleEnterNotify(XEvent *e)
{
	debug("EnterNotify (%d)", e->type);
}

/* 12 */
void
handleExpose(XEvent *e)
{
	//debug("Expose (%d)", e->type);
}

/* 9 */
void
handleFocusIn(XEvent *e)
{
	//debug("FocusIn (%d)", e->type);
}

/* 2 */
void
handleKeyPress(XEvent *e)
{
	//debug("KeyPress (%d)", e->type);
	if ((&e->xkey)->state != MODKEY) return;

	if (XLookupKeysym(&e->xkey, 0) == XK_h) {
		setmfact(-0.02);
	} else if (XLookupKeysym(&e->xkey, 0) == XK_l) {
		setmfact(+0.02);
	} else if (XLookupKeysym(&e->xkey, 0) == XK_j) {
		focusstep(+1);
	} else if (XLookupKeysym(&e->xkey, 0) == XK_k) {
		focusstep(-1);
	} else if (XLookupKeysym(&e->xkey, 0) == XK_r) {
		restart();
	} else if (XLookupKeysym(&e->xkey, 0) == XK_q) {
		quit();
	}
}

/* 3 */
void
handleKeyRelease(XEvent *e)
{
	//debug("KeyRelease! (%d)", e->type);
}

/* 19 */
void
handleMapNotify(XEvent *e)
{
	debug("\033[1;32mmap window %d on window %d (%c= root)%s\033[0m",
			e->xmap.window, e->xmap.event, e->xmap.event == root ? '=' : '!',
			e->xmap.override_redirect ? " with override_redirect" : "");
	attach(e->xmap.window);
}

/* 20 */
void
handleMapRequest(XEvent *e)
{
	debug("MapRequest (%d)", e->type);
}

/* 34 */
void
handleMappingNotify(XEvent *e)
{
	//debug("MappingNotify (%d)", e->type);
}

/* 6 */
void
handleMotionNotify(XEvent *e)
{
	debug("MotionNotify (%d)", e->type);
}

/* 28 */
void
handlePropertyNotify(XEvent *e)
{
	debug("PropertyNotify (%d)", e->type);
}

/* 18 */
void
handleUnmapNotify(XEvent *e)
{
	debug("\033[1;31munmap window %d from window %d (%c= root)%s\033[0m",
			e->xmap.window, e->xmap.event, e->xmap.event == root ? '=' : '!',
			e->xmap.override_redirect ? " with override_redirect" : "");
	detach(wintoclient(e->xdestroywindow.window));
}

/* OTHER FUNCTIONS ---------------------------------------------------------- */

void
attach(Window w)
{
	Client *c;

	/* create client */
	c = malloc(sizeof(Client));
	if (c == NULL) {
		warn("Could not allocate new client.");
		return;
	}
	clients = realloc(clients, ++nc*sizeof(Client *));
	clients[nc-1] = c;
	c->win = w;
	sel = nc-1;
	focus(c);
	tile();
}

void
cleanup(void)
{
	int i;
	for (i = 0; i < nc; i++) {
		detach(clients[i]);
	}
	free(clients);
}

void
detach(Client *c)
{
	int i;

	/* check */
	if (c == NULL) {
		warn("detach(NULL)");
		return;
	}
	if (nc == 0) {
		warn("Attempting to detach from an empty list of clients.");
		return;
	}

	/* remove */
	for (i = 0; i < nc; i++) {
		if (clients[i] == c) {
			free(clients[i]);
			for (; i < nc-1; i++) {
				clients[i] = clients[i+1];
			}
			nc--;
			clients = realloc(clients, nc*sizeof(Client *));
			tile();
			return;
		}
	}
	warn("Attempt to detach a non-existing client.");
}

void
focus(Client *c)
{
	XSetInputFocus(dpy, c->win, RevertToPointerRoot, CurrentTime);
}

void
focusstep(int s)
{
	if (nc == 0) {
		return;
	}
	sel = (sel+nc+s)%nc;
	focus(clients[sel]);
}

void
grabkeys(void)
{
	/* these keys are not passed to a client, but cause a KeyPressed instead
	 * TODO make a list of keys, possibly in config.h
	 */
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_h), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_l), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_j), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_k), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_r), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_q), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
}

void
quit(void)
{
	restarting = false;
	running = false;
}

void
restart(void)
{
	restarting = true;
	running = false;
}

void
run(void)
{
	XEvent e;

	/* event loop (XEvent numerical found in /usr/include/X11/X.h:181++) */
	running = true;
	while (running && !XNextEvent(dpy, &e)) {
		//debug("\033[1;30mrun(): e.type=%d\033[0m", e.type);
		handle[e.type](&e);
	}
}

void
scan(void)
{
	XWindowAttributes wa;
	Window p, r, *wins = NULL;
	unsigned int i, nwins;
	debug("1");
	if (!XQueryTree(dpy, root, &r, &p, &wins, &nwins)) {
		debug("2");
		warn("XQueryTree() failed");
		debug("3");
		return;
	}
	debug("nwins=%d", nwins);
	for (i = 0; i < nwins; i++) {
		debug("4");
		debug("wins[%d] == %d %c= root == %d", i, wins[i],
				wins[i] == root ? '=' : '!', root);
		if (!XGetWindowAttributes(dpy, wins[i], &wa)) {
			debug("5");
			warn("XGetWindowAttributes() failed for window %d", i);
			debug("6");
			continue;
		}
		debug("7");
		attach(wins[i]);
		debug("8");
	}
	debug("9");
}

void
setmfact(float diff)
{
	mfact += diff;
	mfact = max(0.1, mfact);
	mfact = min(0.9, mfact);
	tile();
}

void
setup(void)
{
	/* get colours */
	cbg = BlackPixel(dpy, screen);
	cborder = WhitePixel(dpy, screen);

	/* get root window */
	screen = DefaultScreen(dpy);
	root = RootWindow(dpy, screen);

	/* get screen dimensions */
	sw = DisplayWidth(dpy, screen);
	sh = DisplayHeight(dpy, screen);

	/* set mask of input events to handle */
	XSelectInput(dpy, root, SubstructureNotifyMask|PropertyChangeMask|
			KeyPressMask);
	/*
	XSelectInput(dpy, root, SubstructureRedirectMask|SubstructureNotifyMask|
			ButtonPressMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask|
			StructureNotifyMask|PropertyChangeMask|KeyPressMask);
	*/

	/* for positioning the windows (TODO move to config.h) */
	nmaster = 1;
	mfact = 0.6;

	/* set X error handler */
	xerrorxlib = XSetErrorHandler(xerror);

	/* grab special keys */
	grabkeys();

	/* clients */
	clients = malloc(sizeof(Client *));
}

void
stdlog(FILE *f, char const *format, ...)
{
	if (!DEBUG && f==stdout) return;

	va_list args;
	time_t rawtime;
	struct tm *date;

	/* timestamp */
	time(&rawtime);
	date = localtime(&rawtime);
	fprintf(f, "[%02d:%02d:%02d] stwm: ",
			date->tm_hour, date->tm_min, date->tm_sec);

	/* message */
	va_start(args, format);
	vfprintf(f, format, args);
	va_end(args);

	fprintf(f, "\n");
}

void
tile(void)
{
	int ncm, i, x, w, h;

	if (nc == 0) return;

	/* draw master area */
	ncm = min(nmaster, nc);
	x = 0;
	w = nmaster >= nc ? sw : mfact*sw;
	h = sh/ncm;
	for (i = 0; i < ncm; i++) {
		clients[i]->x = x;
		clients[i]->y = i*h;
		clients[i]->w = w;
		clients[i]->h = (i == ncm-1) ? sh-i*h : h;
		XMoveResizeWindow(dpy, clients[i]->win, clients[i]->x, clients[i]->y,
				clients[i]->w, clients[i]->h);
	}
	if (ncm == nc) return;

	/* draw stack area */
	x = mfact*sw;
	w = sw-x;
	h = sh/(nc-ncm);
	for (; i < nc; i++) {
		clients[i]->x = x;
		clients[i]->y = (i-ncm)*h;
		clients[i]->w = w;
		clients[i]->h = (i == nc-1) ? sh-(i-ncm)*h : h;
		XMoveResizeWindow(dpy, clients[i]->win, clients[i]->x, clients[i]->y,
				clients[i]->w, clients[i]->h);
	}
}

void
unfocus(Client *c)
{
	/* TODO update border colour */
}

Client *
wintoclient(Window w)
{
	int i;
	for (i = 0; i < nc; i++) {
		if (clients[i]->win == w) {
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
	if (ee->error_code == BadWindow) {
		XGetErrorText(dpy, ee->error_code, es, 256);
		warn("Fatal error %d (%s) after request %d",
				ee->error_code, es, ee->error_code);
		return 0;
	}

	/* call default error handler (might call exit) */
	return xerrorxlib(dpy, ee);
}

int
main(int argc, char **argv)
{
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		die("Could not open X.");
	}
	stdlog(stdout, "Starting.");
	setup();
	scan();
	run();
	cleanup();
	stdlog(stdout, "Shutting down.");
	XCloseDisplay(dpy);
	if (restarting) {
		stdlog(stdout, "Restarting.");
		execl("stwm", "stwm", NULL);
	}
	return EXIT_SUCCESS;
}

