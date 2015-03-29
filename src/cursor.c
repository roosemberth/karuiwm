#include "cursor.h"
#include "karuiwm.h"
#include "util.h"

void
cursor_delete(struct cursor *cur)
{
	int unsigned i;

	for (i = 0; i < CURSOR_LAST; ++i)
		XFreeCursor(kwm.dpy, cur->fonts[i]);
	free(cur);
}

struct cursor *
cursor_new(void)
{
	struct cursor *cur;
	XSetWindowAttributes wa;

	cur = smalloc(sizeof(struct cursor), "cursor");
	cur->fonts[CURSOR_NORMAL] = XCreateFontCursor(kwm.dpy, XC_left_ptr);
	cur->fonts[CURSOR_RESIZE_TOP_LEFT] = XCreateFontCursor(kwm.dpy, XC_top_left_corner);
	cur->fonts[CURSOR_RESIZE_TOP_RIGHT] = XCreateFontCursor(kwm.dpy, XC_top_right_corner);
	cur->fonts[CURSOR_RESIZE_BOTTOM_LEFT] = XCreateFontCursor(kwm.dpy, XC_bottom_left_corner);
	cur->fonts[CURSOR_RESIZE_BOTTOM_RIGHT] = XCreateFontCursor(kwm.dpy, XC_bottom_right_corner);
	cur->fonts[CURSOR_RESIZE_LEFT] = XCreateFontCursor(kwm.dpy, XC_left_side);
	cur->fonts[CURSOR_RESIZE_RIGHT] = XCreateFontCursor(kwm.dpy, XC_right_side);
	cur->fonts[CURSOR_RESIZE_TOP] = XCreateFontCursor(kwm.dpy, XC_top_side);
	cur->fonts[CURSOR_RESIZE_BOTTOM] = XCreateFontCursor(kwm.dpy, XC_bottom_side);
	cur->fonts[CURSOR_MOVE] = XCreateFontCursor(kwm.dpy, XC_fleur);
	wa.cursor = cur->fonts[CURSOR_NORMAL];
	XChangeWindowAttributes(kwm.dpy, kwm.root, CWCursor, &wa);
	return cur;
}

int
cursor_get_pos(struct cursor *cur, int *x, int *y)
{
	int i;
	int unsigned ui;
	Window w;
	(void) cur;

	if (!XQueryPointer(kwm.dpy, kwm.root, &w, &w, x, y, &i, &i, &ui)) {
		WARN("XQueryPointer() failed");
		return -1;
	}
	return 0;
}

int
cursor_set_type(struct cursor *cur, enum cursor_type type)
{
	if (type == CURSOR_NORMAL)
		return XUngrabPointer(kwm.dpy, CurrentTime);

	return GrabSuccess == XGrabPointer(kwm.dpy, kwm.root, true, MOUSEMASK,
	                                   GrabModeAsync, GrabModeAsync, None,
	                                   cur->fonts[type], CurrentTime);
}
