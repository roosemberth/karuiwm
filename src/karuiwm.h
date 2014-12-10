#include <X11/Xlib.h>
#include <stdbool.h>

#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))

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

struct client {
	int x, y;
	int unsigned w, h;
	int oldx, oldy, oldw, oldh;
	char name[256];
	Window win;
	bool floating, fullscreen, dialog, dirty;
	int basew, baseh, incw, inch, maxw, maxh, minw, minh;
	int border;
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
	unsigned int mod;
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
	long const *icon_bitfield;
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


int gettiled(struct client ***, struct monitor *);
void moveresizeclient(struct monitor *, struct client *, int, int, int, int);
