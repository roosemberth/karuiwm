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
#define DEBUG 1
#define WBORDER (1)
#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) < (y) ? (y) : (x))
#define debug(...) stdlog(stdout, __VA_ARGS__)
#define warn(...) stdlog(stderr, __VA_ARGS__)
#define die(...) warn(__VA_ARGS__); exit(EXIT_FAILURE)

/* enums */
enum { CURSOR_NORMAL, CURSOR_RESIZE, CURSOR_MOVE, CURSOR_LAST };

/* structs */
typedef struct Client Client;
struct Client {
	int x,y,w,h;
	Client *next;
	Window win;
};

/* functions */
void createClient(void);
void drawClient(Client *);
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
void handleMapNotify(XEvent *);
void handleMappingNotify(XEvent *);
void handleMapRequest(XEvent *);
void handleMotionNotify(XEvent *);
void handlePropertyNotify(XEvent *);
void handleUnmapNotify(XEvent *);
void removeClient(void);
void run(void);
void setmfact(float);
void setup(void);
void stdlog(FILE *, char const *, ...);
void tile(void);

/* handler functions, in an array to allow easier access */
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

void
createClient(void)
{
	Client *c, *cn;

	/* create client */
	cn = malloc(sizeof(Client));
	if (cn == NULL) {
		warn("Could not allocate new client.\n");
		return;
	}

	/* initialise client (TODO initial dimension values) */
	cn->win = XCreateSimpleWindow(dpy, root, 1, 1, 1, 1, WBORDER, cborder, cbg);
	cn->next = NULL;

	/* integrate client */
	if (clients == NULL) {
		clients = cn;
	} else {
		for (c = clients; c->next != NULL; c = c->next);
		c->next = cn;
	}

	/* arrange windows */
	tile();
}

void
drawClient(Client *c)
{
	XMapWindow(dpy, c->win);
}

void
handleButtonPress(XEvent *e)
{
	debug("ButtonPress! (type=%d)\n", e->type);
}

void
handleClientMessage(XEvent *e)
{
	debug("ClientMessage! (type=%d)\n", e->type);
}

void
handleConfigureRequest(XEvent *e)
{
	debug("ConfigureRequest! (type=%d)\n", e->type);
}

void
handleConfigureNotify(XEvent *e)
{
	debug("ConfigureNotify! (type=%d)\n", e->type);
}

void
handleCreateNotify(XEvent *e)
{
	debug("CreateNotify! (type=%d)\n", e->type);
}

void
handleDestroyNotify(XEvent *e)
{
	debug("DestroyNotify! (type=%d)\n", e->type);
}

void
handleEnterNotify(XEvent *e)
{
	debug("EnterNotify! (type=%d)\n", e->type);
}

void
handleExpose(XEvent *e)
{
	debug("Expose! (type=%d)\n", e->type);
}

void
handleFocusIn(XEvent *e)
{
	debug("FocusIn! (type=%d)\n", e->type);
}

void
handleKeyPress(XEvent *e)
{
	switch (XLookupKeysym(&e->xkey, 0)) {
		case XK_space:
			createClient();
			break;
		case XK_q:
			removeClient();
			break;
		case XK_h:
			setmfact(-0.02);
			break;
		case XK_l:
			setmfact(+0.02);
			break;
	}
}

void
handleMapNotify(XEvent *e)
{
	XMapEvent me;
	Window w;

	me = e->xmap;
	w = me.window;
	//XSetInputFocus(dpy, w, RevertToNone, CurrentTime);
}

void
handleMappingNotify(XEvent *e)
{
	debug("MappingNotify! (type=%d)\n", e->type);
}

void
handleMapRequest(XEvent *e)
{
	debug("MapRequest! (type=%d)\n", e->type);
}

void
handleMotionNotify(XEvent *e)
{
	debug("MotionNotify! (type=%d)\n", e->type);
}

void
handlePropertyNotify(XEvent *e)
{
	debug("PropertyNotify! (type=%d)\n", e->type);
}

void
handleUnmapNotify(XEvent *e)
{
	debug("UnmapNotify! (type=%d)\n", e->type);
}

void
removeClient(void)
{
	Client *c;
	if (clients == NULL) {
		running = false;
	} else if (clients->next == NULL) {
		XDestroyWindow(dpy, clients->win);
		clients = NULL;
	} else {
		for (c = clients; c->next->next != NULL; c = c->next);
		XDestroyWindow(dpy, c->next->win);
		free(c->next);
		c->next = false;
	}
	tile();
}

void
run(void)
{
	XEvent e;

	/* event loop */
	running = true;
	while (running && !XNextEvent(dpy, &e)) {
		//debug("run(): e.type=%d\n", e.type);
		handle[e.type](&e);
	}
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
	XSelectInput(dpy, root, SubstructureNotifyMask|/*EnterWindowMask|*/KeyPressMask);
	/*
	XSelectInput(dpy, root, SubstructureRedirectMask|SubstructureNotifyMask|
			ButtonPressMask|PointerMotionMask|EnterWindowMask|LeaveWindowMask|
			StructureNotifyMask|PropertyChangeMask|KeyPressMask);
	*/

	/* for positioning the windows */
	nmaster = 1;
	mfact = 0.6;
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
		XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w-2*WBORDER, c->h-2*WBORDER);
		drawClient(c);
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
		XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w-2*WBORDER, c->h-2*WBORDER);
		drawClient(c);
	}
}

int
main(int argc, char **argv)
{
	/* open the display */
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		die("Could not open X.\n");
	}

	setup();
	run();

	/* close window */
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}

