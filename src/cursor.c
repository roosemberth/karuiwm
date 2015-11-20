#include "cursor.h"
#include "karuiwm.h"
#include "util.h"

#include <stdbool.h>

void
cursor_delete(struct cursor *cur)
{
	int unsigned i;

	for (i = 0; i < CURSOR_LAST; ++i)
		XFreeCursor(karuiwm.dpy, cur->fonts[i]);
	free(cur);
}

struct cursor *
cursor_new(void)
{
	struct cursor *cur;
	XSetWindowAttributes wa;

	cur = smalloc(sizeof(struct cursor), "cursor");
	cur->fonts[CURSOR_NORMAL] = XCreateFontCursor(karuiwm.dpy, XC_left_ptr);
	cur->fonts[CURSOR_RESIZE_TOP_LEFT] = XCreateFontCursor(karuiwm.dpy, XC_top_left_corner);
	cur->fonts[CURSOR_RESIZE_TOP_RIGHT] = XCreateFontCursor(karuiwm.dpy, XC_top_right_corner);
	cur->fonts[CURSOR_RESIZE_BOTTOM_LEFT] = XCreateFontCursor(karuiwm.dpy, XC_bottom_left_corner);
	cur->fonts[CURSOR_RESIZE_BOTTOM_RIGHT] = XCreateFontCursor(karuiwm.dpy, XC_bottom_right_corner);
	cur->fonts[CURSOR_RESIZE_LEFT] = XCreateFontCursor(karuiwm.dpy, XC_left_side);
	cur->fonts[CURSOR_RESIZE_RIGHT] = XCreateFontCursor(karuiwm.dpy, XC_right_side);
	cur->fonts[CURSOR_RESIZE_TOP] = XCreateFontCursor(karuiwm.dpy, XC_top_side);
	cur->fonts[CURSOR_RESIZE_BOTTOM] = XCreateFontCursor(karuiwm.dpy, XC_bottom_side);
	cur->fonts[CURSOR_MOVE] = XCreateFontCursor(karuiwm.dpy, XC_fleur);
	wa.cursor = cur->fonts[CURSOR_NORMAL];
	XChangeWindowAttributes(karuiwm.dpy, karuiwm.root, CWCursor, &wa);
	return cur;
}

int
cursor_get_pos(struct cursor *cur, int *x, int *y)
{
	int i;
	int unsigned ui;
	Window w;
	(void) cur;

	if (!XQueryPointer(karuiwm.dpy, karuiwm.root, &w, &w, x, y, &i, &i, &ui)) {
		WARN("XQueryPointer() failed");
		return -1;
	}
	return 0;
}

int
cursor_set_type(struct cursor *cur, enum cursor_type type)
{
	if (type == CURSOR_NORMAL)
		return XUngrabPointer(karuiwm.dpy, CurrentTime);

	return XGrabPointer(karuiwm.dpy, karuiwm.root, true, MOUSEMASK,
	                    GrabModeAsync, GrabModeAsync, None,
	                    cur->fonts[type], CurrentTime)
	       == GrabSuccess;
}
