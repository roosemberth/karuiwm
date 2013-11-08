#include <X11/Xlib.h>
#include <X11/cursorfont.h>
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
	Client *next;
	Window win;
};

/* functions */
void attach(Window);
void cleanup(void);
void detach(Client *);
void run(void);
void setmfact(float);
void setup(void);
void stdlog(FILE *, char const *, ...);
void tile(void);
Client *wintoclient(Window);
int xerror(Display *, XErrorEvent *);
int (*xerrorxlib)(Display *, XErrorEvent *);

/* handler functions */
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
int cbg, cborder;
Cursor cursor[CURSOR_LAST]; /* TODO */
Display *dpy;
bool running;
Window root;
int screen;
int sw, sh; /* screen dimensions */
Client *clients;
int nmaster;
float mfact;

/* HANDLER FUNCTIONS -------------------------------------------------------- */

void
handleButtonPress(XEvent *e)
{
	debug("ButtonPress (%d)", e->type);
}

void
handleClientMessage(XEvent *e)
{
	debug("ClientMessage (%d)", e->type);
}

void
handleConfigureRequest(XEvent *e)
{
	debug("ConfigureRequest (%d)", e->type);
}

void
handleConfigureNotify(XEvent *e)
{
	debug("ConfigureNotify (%d)", e->type);
	/* TODO */
}

void
handleCreateNotify(XEvent *e)
{
	//debug("CreateNotify! (%d)", e->type);
}

void
handleDestroyNotify(XEvent *e)
{
	//debug("DestroyNotify! (%d)", e->type);
}

void
handleEnterNotify(XEvent *e)
{
	debug("EnterNotify (%d)", e->type);
}

void
handleExpose(XEvent *e)
{
	debug("Expose (%d)", e->type);
}

void
handleFocusIn(XEvent *e)
{
	debug("FocusIn (%d)", e->type);
}

void
handleKeyPress(XEvent *e)
{
	debug("KeyPress (%d)", e->type);
	if ((&e->xkey)->state != MODKEY) return;

	if (XLookupKeysym(&e->xkey, 0) == XK_h) {
		setmfact(-0.02);
	} else if (XLookupKeysym(&e->xkey, 0) == XK_l) {
		setmfact(+0.02);
	} else if (XLookupKeysym(&e->xkey, 0) == XK_q) {
		running = false;
	}
}

void
handleKeyRelease(XEvent *e)
{
	debug("KeyRelease! (%d)", e->type);
}

void
handleMapNotify(XEvent *e)
{
	//debug("MapNotify (%d)", e->type);
	attach(e->xmap.window);
}

void
handleMapRequest(XEvent *e)
{
	debug("MapRequest (%d)", e->type);
}

void
handleMappingNotify(XEvent *e)
{
	debug("MappingNotify (%d)", e->type);
}

void
handleMotionNotify(XEvent *e)
{
	debug("MotionNotify (%d)", e->type);
}

void
handlePropertyNotify(XEvent *e)
{
	debug("PropertyNotify (%d)", e->type);
}

void
handleUnmapNotify(XEvent *e)
{
	//debug("UnmapNotify (%d)", e->type);
	detach(wintoclient(e->xdestroywindow.window));
}

/* OTHER FUNCTIONS ---------------------------------------------------------- */

void
attach(Window w)
{
	Client *c, *cn;

	/* create client */
	cn = malloc(sizeof(Client));
	if (cn == NULL) {
		warn("Could not allocate new client.");
		return;
	}

	/* initialise client */
	cn->win = w;
	cn->next = NULL;

	/* integrate client */
	if (clients == NULL) {
		clients = cn;
	} else {
		for (c = clients; c->next != NULL; c = c->next);
		c->next = cn;
	}
	tile();
}

void
cleanup(void)
{
	Client *c;
	for (c = clients; c != NULL; c = c->next) {
		detach(c);
	}
}

void
detach(Client *c)
{
	Client *cp;

	if (clients == NULL || c == NULL) {
		warn("Attempting window detaching with NULL state.");
	} else if (clients == c) {
		clients = c->next;
		free(c);
	} else {
		for (cp = clients; cp->next != NULL; cp = cp->next) {
			if (cp->next == c) {
				cp->next = c->next;
				free(c);
				break;
			}
		}
	}
	tile();
}

void
grabkeys(void)
{
	/* these keys are not passed to a client, but cause a KeyPressed instead */
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_h), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_l), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
	XGrabKey(dpy, XKeysymToKeycode(dpy, XK_q), MODKEY, root, true,
			GrabModeAsync, GrabModeAsync);
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
	stdlog(stderr, "Received shutdown signal.");
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
	XSelectInput(dpy, root, SubstructureNotifyMask|KeyPressMask);
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
	int nc, ncm, i, x, w, h;
	Client *c;

	/* get number of windows in master area */
	for (nc = 0, c = clients; c != NULL; c = c->next, nc++);
	if (nc == 0) return;

	/* draw master area */
	ncm = min(nmaster, nc);
	x = 0;
	w = nmaster >= nc ? sw : mfact*sw;
	h = sh/ncm;
	for (i = 0, c = clients; i < ncm; i++, c = c->next) {
		c->x = x;
		c->y = i*h;
		c->w = w;
		c->h = (i == ncm-1) ? sh-i*h : h;
		XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	}
	if (ncm == nc) return;

	/* draw stack area */
	x = mfact*sw;
	w = sw-x;
	h = sh/(nc-ncm);
	for (; i < nc; i++, c = c->next) {
		c->x = x;
		c->y = (i-ncm)*h;
		c->w = w;
		c->h = (i == nc-1) ? sh-(i-ncm)*h : h;
		XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w, c->h);
	}
}

Client *
wintoclient(Window w)
{
	Client *c;

	for (c = clients; c != NULL; c = c->next) {
		if (c->win == w) {
			return c;
		}
	}
	return NULL;
}

int
xerror(Display *dpy, XErrorEvent *ee)
{
	/* only display error on this error instead of crashing */
	if (ee->error_code == BadWindow) {
		warn("Fatal error (request code=%d, error code=%d)",
				ee->request_code, ee->error_code);
		return 0;
	}

	/* call default error handler (might call exit) */
	return xerrorxlib(dpy, ee);
}

int
main(int argc, char **argv)
{
	/* open the display */
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		die("Could not open X.");
	}

	setup();
	run();
	cleanup();

	stdlog(stdout, "Shutting down stwm.");

	/* close window */
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}

