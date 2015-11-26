#ifndef _KARUIWM_H
#define _KARUIWM_H

#include <stdbool.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "action.h"

/* macros */
#define CLIENTMASK (EnterWindowMask | PropertyChangeMask | StructureNotifyMask)
#define BUTTONMASK (ButtonPressMask | ButtonReleaseMask)
#define MOUSEMASK (BUTTONMASK | PointerMotionMask)

#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

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
#define BUFSIZE_HOSTNAME 128

Atom atoms[ATOM_LAST], netatoms[NETATOM_LAST];
struct {
	Display *dpy;
	Window root;
	int screen;
	int xfd;
	Colormap cm;
	bool running;
	bool restarting;
	struct focus *focus;
	struct session *session;
	struct cursor *cursor;
	struct {
		char HOSTNAME[BUFSIZE_HOSTNAME];
		char *HOME;
		char const *APPNAME;
	} env;
} karuiwm;
struct action *actions;
size_t nactions;

#endif /* ndef _KARUIWM_H */
