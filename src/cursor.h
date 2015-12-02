#ifndef _KARUIWM_CURSOR_H
#define _KARUIWM_CURSOR_H

#include <X11/Xlib.h>
#include <X11/cursorfont.h>

/* enumerations */
enum cursor_type { CURSOR_NORMAL, CURSOR_MOVE,
                   CURSOR_RESIZE_TOP_LEFT, CURSOR_RESIZE_TOP_RIGHT,
                   CURSOR_RESIZE_BOTTOM_LEFT, CURSOR_RESIZE_BOTTOM_RIGHT,
                   CURSOR_RESIZE_LEFT, CURSOR_RESIZE_RIGHT,
                   CURSOR_RESIZE_TOP, CURSOR_RESIZE_BOTTOM,
                   CURSOR_LAST };

/* structures */
struct cursor {
	Cursor fonts[CURSOR_LAST];
};

/* functions */
void cursor_delete(struct cursor *cur);
int cursor_get_pos(struct cursor *cur, int *x, int *y);
struct cursor *cursor_new(void);
int cursor_set_type(struct cursor *cur, enum cursor_type type);

#endif /* ndef _KARUIWM_CURSOR_H */
