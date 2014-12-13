#ifndef _KARUIWM_H
#define _KARUIWM_H

#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdio.h>

/* macros */
#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define ERROR(...)   print(stderr, LOG_ERROR, __VA_ARGS__)
#define WARN(...)    print(stderr, LOG_WARN, __VA_ARGS__)
#define NOTE(...)    print(stdout, LOG_NORMAL, __VA_ARGS__)
#define VERBOSE(...) print(stdout, LOG_VERBOSE, __VA_ARGS__)
#define DEBUG(...)   print(stdout, LOG_DEBUG, __VA_ARGS__)

/* enumerations */
enum log_level { LOG_FATAL, LOG_ERROR, LOG_WARN, LOG_NORMAL, LOG_VERBOSE,
                 LOG_DEBUG };
enum { WMProtocols, WMDeleteWindow, WMState, WMTakeFocus, WMLAST };
enum { NetActiveWindow, NetSupported, NetWMName, NetWMState,
		NetWMStateFullscreen, NetWMWindowType, NetWMWindowTypeDialog, NetLAST };

/* structures/unions */
union argument {
	int i;
	float f;
	void const *v;
};

struct button {
	int unsigned mod;
	int unsigned button;
	void (*func)(union argument const *);
	union argument const arg;
};

struct {
	GC gc;
	unsigned int sd; /* screen depth */
	struct {
		int ascent, descent, height;
		XFontStruct *xfontstruct;
		XFontSet xfontset;
	} font;
} dc;

struct dimension {
	int x, y;
	int unsigned w, h;
};

struct key {
	int unsigned mod;
	KeySym key;
	void (*func)(union argument const *);
	union argument const arg;
};

struct monitor {
	struct workspace *selws;
	int x, y, wx, wy;
	int unsigned w, h, ww, wh;
	/* prefix with w: workspace dimension */
};

struct layout {
	int long unsigned const *icon_bitfield;
	void (*func)(struct monitor *);
	Pixmap icon_norm, icon_sel;
	int w, h;
};

struct workspace {
	struct client **clients, **stack, *selcli;
	Window wsmbox;
	Pixmap wsmpm;
	size_t nc, ns, nmaster;
	float mfact;
	char name[256];
	int x, y;
	int ilayout;
	bool dirty;
	struct monitor *mon;
};

struct {
	struct workspace *target;
	XSetWindowAttributes wa;
	bool active;
} wsm;

struct {
	Display *dpy;
} kwm;

/* functions */
int gettiled(struct client ***, struct monitor *);
void grabbuttons(struct client *, bool);
void moveresizeclient(struct monitor *, struct client *, int, int, int unsigned, int unsigned);
void print(FILE *f, enum log_level level, char const *format, ...);

/* variables */
Atom wmatom[WMLAST], netatom[NetLAST];

#endif /* _KARUIWM_H */
