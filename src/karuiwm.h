#ifndef _KARUIWM_H
#define _KARUIWM_H

#include <X11/Xlib.h>
#include <stdbool.h>

/* macros */
#define BORDERWIDTH 1          /* window border width */
#define CBORDERNORM 0x222222   /* normal windows */
#define CBORDERSEL  0xAFD700   /* selected windows */

#define CLIENTMASK (EnterWindowMask | PropertyChangeMask | StructureNotifyMask)
#define BUTTONMASK (ButtonPressMask | ButtonReleaseMask)
#define MODKEY Mod1Mask

#define APPNAME "karuiwm"
#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

/* enumerations */
enum { WMProtocols, WMDeleteWindow, WMState, WMTakeFocus, WMLAST };
enum { NetActiveWindow, NetSupported, NetWMName, NetWMState,
       NetWMStateFullscreen, NetWMWindowType, NetWMWindowTypeDialog, NetLAST };

/* unions, structures */
union argument {
	int i;
	float f;
	void *v;
};

struct button {
	int unsigned mod;
	int unsigned button;
	void (*func)(union argument const *);
	union argument const arg;
};

struct key {
	int unsigned mod;
	KeySym key;
	void (*func)(union argument const *);
	union argument const arg;
};

/* variables */
Atom wmatom[WMLAST], netatom[NetLAST];
struct {
	Display *dpy;
	Window root;
} kwm;

#endif /* _KARUIWM_H */
