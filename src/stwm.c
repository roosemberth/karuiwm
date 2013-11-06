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
void addClient(void);
void drawClient(Client *);
void handleButton(XEvent *);
void handleKeyPress(XEvent *);
void removeClient(void);
void run(void);
void setup(void);
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
int count;

void
addClient(void)
{
	Client *c, *cn;

	/* create client */
	cn = malloc(sizeof(Client));
	if (cn == NULL) {
		warn("could not allocate new client\n");
		return;
	}

	/* initialise client */
	cn->x = 10*count; /* TODO */
	cn->y = 10*count;
	++count;
	cn->w = 200;
	cn->h = 140;
	cn->win = XCreateSimpleWindow(dpy, root, cn->x, cn->y, cn->w, cn->h, WBORDER, cborder, cbg);
	cn->next = NULL;

	/* draw client */
	drawClient(cn);

	/* integrate client */
	if (clients == NULL) {
		clients = cn;
	} else {
		for (c = clients; c->next != NULL; c = c->next);
		c->next = cn;
	}
}

void
drawClient(Client *c)
{
	XMapWindow(dpy, c->win);
}

void
handleButton(XEvent *e)
{
	printf("button pressed\n");
}

void
handleKeyPress(XEvent *e)
{
	switch (XLookupKeysym(&e->xkey, 0)) {
		case XK_space:
			addClient();
			break;
		case XK_d:
			removeClient();
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
	} else {
		for (c = clients; c->next->next != NULL; c = c->next);
		XDestroyWindow(dpy, c->next->win);
		free(c->next);
		c->next = false;
		drawClient(c);
	}
	--count;
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
	count = 0;
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

