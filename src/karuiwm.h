#ifndef _KARUIWM_H
#define _KARUIWM_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include "actions.h"

/* macros */
#define BORDERWIDTH 1          /* window border width */
#define CBORDERNORM 0x222222   /* normal windows */
#define CBORDERSEL  0x00FF00   /* selected windows */

#define CLIENTMASK (EnterWindowMask | PropertyChangeMask | StructureNotifyMask)
#define BUTTONMASK (ButtonPressMask | ButtonReleaseMask)
#define MOUSEMASK (BUTTONMASK | PointerMotionMask)
#define MODKEY Mod1Mask

#define APPNAME "karuiwm"
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

/* enumerations */
enum direction { LEFT, RIGHT, UP, DOWN, NO_DIRECTION };
enum runstate { STOPPED, STARTING, RUNNING, STOPPING, RESTARTING };

/* structures */
struct button {
	int unsigned mod;
	int unsigned button;
	void (*func)(union argument *, Window win);
	union argument arg;
};

struct key {
	int unsigned mod;
	KeySym key;
	void (*func)(union argument *);
	union argument arg;
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
	enum runstate state;
	struct focus *focus;
	struct session *session;
	struct cursor *cursor;
	struct {
		char HOSTNAME[BUFSIZE_HOSTNAME];
		char *HOME;
	} env;
} karuiwm;

#endif /* ndef _KARUIWM_H */
