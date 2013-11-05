#include <X11/Xlib.h>
#include <stdio.h>  /* fprintf() */
#include <stdlib.h> /* NULL, EXIT_FAILURE */
#include <unistd.h> /* sleep() */
#include <stdbool.h>

#define WBORDER (2)
#define HEIGHT (768-(2*WBORDER))
#define WIDTH (1366-(2*WBORDER))

int
main(int argc, char **argv)
{
	Display *dpy;
	Window w, root;
	int cblack, cwhite;
	XEvent e;
	GC gc;
	bool running = true;

	/* open the display */
	dpy = XOpenDisplay(NULL);
	if (dpy == NULL) {
		fprintf(stderr, "Could not open X.\n");
		exit(EXIT_FAILURE);
	}

	/* get some colours */
	cblack = BlackPixel(dpy, DefaultScreen(dpy));
	cwhite = WhitePixel(dpy, DefaultScreen(dpy));

	/* get root window */
	root = DefaultRootWindow(dpy);

	/* create a window (..., ..., top left corner, dimensions, border width&colour, bg colour) */
	w = XCreateSimpleWindow(dpy, root, 0, 0, WIDTH, HEIGHT, WBORDER, cwhite, cblack);

	/* create a graphics context */
	gc = XCreateGC(dpy, w, 0, NULL);

	/* map window to display (make it appear); then event loop */
	XMapWindow(dpy, w);
	XSelectInput(dpy, w, ExposureMask|KeyPressMask|ButtonPressMask);
	while (running) {
		XNextEvent(dpy, &e);
		switch (e.type) {
			case Expose:
				printf("Expose\n");
				/* draw a line, then flush it */
				XSetForeground(dpy, gc, cwhite);
				XDrawLine(dpy, w, gc, 0, 0, 300, 200);
				XFlush(dpy);
				break;
			case KeyPress:
				printf("KeyPress\n");
				break;
			default:
				printf("default\n");
				running = false;
				break;
		}
	}

	/* close window */
	XCloseDisplay(dpy);

	return EXIT_SUCCESS;
}

