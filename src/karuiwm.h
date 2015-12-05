#ifndef _KARUIWM_H
#define _KARUIWM_H

#include <stdbool.h>
#include <X11/Xlib.h>
#include "action.h"

/* macros */
#define CLIENTMASK (EnterWindowMask | PropertyChangeMask | StructureNotifyMask)
#define BUTTONMASK (ButtonPressMask | ButtonReleaseMask)
#define MOUSEMASK (BUTTONMASK | PointerMotionMask)

/* atoms */
#define _NET_WM_STATE_REMOVE 0
#define _NET_WM_STATE_ADD 1
#define _NET_WM_STATE_TOGGLE 2

enum atom_type {
	WM_PROTOCOLS,
	WM_DELETE_WINDOW,
	WM_STATE,
	WM_TAKE_FOCUS,
	ATOM_LAST
};

enum netatom_type {
	_NET_ACTIVE_WINDOW,
	_NET_SUPPORTED,
	_NET_WM_NAME,
	_NET_WM_STATE,
	_NET_WM_STATE_FULLSCREEN,
	_NET_WM_STATE_HIDDEN,
	_NET_WM_WINDOW_TYPE,
	_NET_WM_WINDOW_TYPE_DIALOG,
	_NET_WM_STRUT,
	_NET_WM_STRUT_PARTIAL,
	NETATOM_LAST
};

/* variables */
Atom atoms[ATOM_LAST], netatoms[NETATOM_LAST];
struct {
	Display *dpy;
	Window root;
	int screen;
	int xfd;
	Colormap cm;
	bool running, restarting;
	struct focus *focus;
	struct session *session;
	struct cursor *cursor;
	struct {
		char *HOME;
		char const *APPNAME;
	} env;
	XEvent *event;
} karuiwm;

/* X event handler */
void handle_event(XEvent *xe);

#endif /* ndef _KARUIWM_H */
