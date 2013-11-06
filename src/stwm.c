#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/keysym.h>
#include <stdio.h>  /* fprintf() */
#include <stdlib.h> /* NULL, EXIT_FAILURE */
#include <unistd.h> /* sleep() */
#include <stdbool.h>
#include <stdarg.h>

/* macros */
#define WBORDER (1)
#define die(ARGS) warn(ARGS); exit(EXIT_FAILURE)
#define min(x,y) ((x) < (y) ? (x) : (y))
#define max(x,y) ((x) < (y) ? (y) : (x))

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
void handleButton(XEvent *);
void handleKeyPress(XEvent *);
void removeClient(void);
void run(void);
void setmfact(float);
void setup(void);
void tile(void);
void warn(char const *, ...);

/* handler functions, in an array to allow easier access */
void (*handlers[LASTEvent])(XEvent *e) = {
	[ButtonPress] = handleButton,
	[KeyPress] = handleKeyPress
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
		warn("could not allocate new client\n");
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
handleButton(XEvent *e)
{
	/* TODO */
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
		handlers[e.type](&e);
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
	XSelectInput(dpy, root, KeyPressMask|ButtonPressMask);

	/* for positioning the windows */
	nmaster = 1;
	mfact = 0.6;
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
		c->h = h;
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
		c->h = h;
		XMoveResizeWindow(dpy, c->win, c->x, c->y, c->w-2*WBORDER, c->h-2*WBORDER);
		drawClient(c);
	}
}

void
warn(char const *format, ...)
{
	va_list args;
	va_start(args, format);
	vfprintf(stderr, format, args);
	va_end(args);
}

int
main(int argc, char **argv)
{
	/* open the display */
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "Could not open X.\n");
		exit(EXIT_FAILURE);
	}

	setup();
	run();

	/* close window */
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}

