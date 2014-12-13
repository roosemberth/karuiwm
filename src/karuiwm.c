#include "karuiwm.h"
#include "client.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <time.h>
#include <locale.h>
#include <signal.h>
#include <errno.h>
#include <sys/wait.h>
#include <sys/select.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#include <X11/Xproto.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xinerama.h>

/* macros */
#define APPNAME "karuiwm"
#define APPVERSION "0.1"
#define BUTTONMASK (ButtonPressMask|ButtonReleaseMask)
#define CLIENTMASK (EnterWindowMask|PropertyChangeMask|StructureNotifyMask)
#define INTERSECT(MON, X, Y, W, H) \
	((MAX(0, MIN((X)+(W),(MON)->x+(MON)->w) - MAX((MON)->x, X))) * \
	 (MAX(0, MIN((Y)+(H),(MON)->y+(MON)->h) - MAX((MON)->y, Y))))
#define MOUSEMASK (BUTTONMASK|PointerMotionMask)
#define FATAL(...) print(stderr, LOG_FATAL, __VA_ARGS__)

/* enums */
enum { CurNormal, CurResize, CurMove, CurLAST };
enum { Left, Right, Up, Down, NoDirection };
enum DMenuState { DMenuInactive, DMenuSpawn, DMenuRename, DMenuView, DMenuSend,
		DMenuSendFollow, DMenuClients, DMenuLAST };

/* functions */
static void arrange(struct monitor *);
static void attachclient(struct workspace *, struct client *);
static void attachws(struct workspace *);
static void buttonpress(XEvent *);
static bool checksizehints(struct client *, int unsigned *, int unsigned *);
static void cleanup(char **);
static void clientmessage(XEvent *);
static void configurerequest(XEvent *);
static void configurenotify(XEvent *);
static void detachclient(struct client *);
static void detachmon(struct monitor *);
static void detachws(struct workspace *);
static void dmenu(union argument const *);
static void dmenueval(void);
static void enternotify(XEvent *);
static void expose(XEvent *);
static void focusin(XEvent *);
static void grabkeys(void);
static struct monitor *initmon(void);
static Pixmap initpixmap(long const *, long const, long const);
static struct workspace *initws(int, int);
static void initwsmbox(struct workspace *);
static void keypress(XEvent *);
static void keyrelease(XEvent *);
static void killclient(union argument const *);
static bool locateclient(struct workspace **, struct client **, unsigned int *, Window const, char const *);
static bool locatemon(struct monitor **, unsigned int *, struct workspace const *);
static struct monitor *locaterect(int, int, int, int);
static bool locatews(struct workspace **, unsigned int *, int, int, char const *);
static void mappingnotify(XEvent *);
static void maprequest(XEvent *);
static void movemouse(union argument const *);
static void pop(struct workspace *, struct client *);
static void propertynotify(XEvent *);
static void push(struct workspace *, struct client *);
static void quit(union argument const *);
static void reltoxy(int *, int *, struct workspace *, int);
static void renamews(struct workspace *, char const *);
static void renderwsmbox(struct workspace *);
static void resizeclient(struct client *, int unsigned, int unsigned);
static void resizemouse(union argument const *);
static void restart(union argument const *);
static void restoresession(char const *sessionfile);
static void run(void);
static void savesession(char **sessionfile);
static bool sendevent(struct client *, Atom);
static void sendfollowclient(union argument const *);
static void setclientmask(struct monitor *, bool);
static void setfloating(struct client *, bool);
static void setfullscreen(struct client *, bool);
static void setmfact(union argument const *);
static void setnmaster(union argument const *);
static void setpad(union argument const *);
static void setup(char const *sessionfile);
static void setupatoms(void);
static void setupfont(void);
static void setuplayouts(void);
static void setupsession(char const *sessionfile);
static void setupwsm(void);
static void setws(struct monitor *, int, int);
static void shiftclient(union argument const *);
static void shiftws(union argument const *);
static void showclient(struct monitor *, struct client *);
static void showws(struct monitor *, struct workspace *);
static void sigchld(int);
static void spawn(union argument const *);
static void stepfocus(union argument const *);
static void steplayout(union argument const *);
static void stepmon(union argument const *);
static void stepws(union argument const *);
static void stepwsmbox(union argument const *arg);
static void termclient(struct client *);
static void termlayout(struct layout *);
static void termmon(struct monitor *);
static void termws(struct workspace *);
static void termwsmbox(struct workspace *);
static void togglefloat(union argument const *);
static void togglepad(union argument const *);
static void togglewsm(union argument const *);
static size_t unifyscreens(XineramaScreenInfo **, size_t);
static void unmapnotify(XEvent *);
static void updateclient(struct client *);
static void updatefocus(void);
static void updategeom(void);
static void updatemon(struct monitor *, int, int, unsigned int, unsigned int);
static void updatetransient(struct client *);
static void updatewsm(void);
static void updatewsmbox(struct workspace *);
static void updatewsmpixmap(struct workspace *);
static void viewmon(struct monitor *);
static void viewws(union argument const *);
static int xerror(Display *, XErrorEvent *);
static int (*xerrorxlib)(Display *, XErrorEvent *);
static void zoom(union argument const *);

/* event handlers, as array to allow O(1) access; codes in X.h */
static void (*handle[LASTEvent])(XEvent *) = {
	[ButtonPress]      = buttonpress,      /* 4*/
	[ClientMessage]    = clientmessage,    /*33*/
	[ConfigureNotify]  = configurenotify,  /*22*/
	[ConfigureRequest] = configurerequest, /*23*/
	[EnterNotify]      = enternotify,      /* 7*/
	[Expose]           = expose,           /*12*/
	[FocusIn]          = focusin,          /* 9*/
	[KeyPress]         = keypress,         /* 2*/
	[KeyRelease]       = keyrelease,       /* 3*/
	[MapRequest]       = maprequest,       /*20*/
	[MappingNotify]    = mappingnotify,    /*34*/
	[PropertyNotify]   = propertynotify,   /*28*/
	[UnmapNotify]      = unmapnotify       /*18*/
};

/* variables */
static bool running, restarting;              /* application state */
static int screen;                            /* screen */
static Window root;                           /* root window */
static Cursor cursor[CurLAST];                /* cursors */
static struct workspace **workspaces;                /* all workspaces */
static struct monitor **monitors, *selmon;           /* all/selected monitor(s) */
static unsigned int nws, nmon;                /* # of workspaces/monitors */
static int dmenu_out;                         /* dmenu output file descriptor */
static enum DMenuState dmenu_state;           /* dmenu state */
static int nlayouts;                          /* number of cyclable layouts */
static struct client *pad;                           /* scratchpad window */
static struct monitor *pad_mon;                      /* monitor with scratchpad */
static enum log_level log_level;

/* configuration */
#include "layout.h"
#include "config.h"

/* function implementation */

void
arrange(struct monitor *mon)
{
	struct client *c;
	unsigned int i;

	setclientmask(mon, false);
	layouts[mon->selws->ilayout].func(mon);
	for (i = 0; i < mon->selws->nc; i++) {
		c = mon->selws->clients[i];
		if (c->floating) {
			client_move(mon, c, c->x, c->y);
		}
	}
	setclientmask(mon, true);
	mon->selws->dirty = false;
	mon->selws->mon = mon;
}

void
attachclient(struct workspace *ws, struct client *c)
{
	unsigned int i, pos;

	/* add to list */
	ws->clients = realloc(ws->clients, ++ws->nc*sizeof(struct client *));
	if (!ws->clients) {
		FATAL("could not allocate %zu bytes for client list",
		      ws->nc*sizeof(struct client *));
		exit(EXIT_FAILURE);
	}
	pos = 0;
	if (ws->nc > 1) {
		for (i = 0; i < ws->nc-1; i++) {
			if (ws->clients[i] == ws->selcli) {
				pos = i+1;
				for (i = ws->nc-1; i > pos; i--) {
					ws->clients[i] = ws->clients[i-1];
				}
				break;
			}
		}
	}
	ws->clients[pos] = c;

	/* add to stack */
	push(ws, c);

	/* update layout if on a selected workspace */
	if (!c->floating) {
		ws->dirty = true;
	}
}

void
attachmon(struct monitor *mon)
{
	monitors = realloc(monitors, ++nmon*sizeof(struct monitor *));
	if (!monitors) {
		FATAL("could not allocate %zu bytes for monitor list",
		      sizeof(struct monitor));
		exit(EXIT_FAILURE);
	}
	monitors[nmon-1] = mon;
}

void
attachws(struct workspace *ws)
{
	unsigned int i;

	workspaces = realloc(workspaces, ++nws*sizeof(struct workspace *));
	if (!workspaces) {
		FATAL("could not allocate %zu bytes for workspace list",
		      nws*sizeof(struct workspace *));
		exit(EXIT_FAILURE);
	}
	for (i = nws-1; i >= 0; i--) {
		if (i == 0 || strcasecmp(workspaces[i-1]->name, ws->name) < 0) {
			workspaces[i] = ws;
			break;
		}
		workspaces[i] = workspaces[i-1];
	}
}

void
buttonpress(XEvent *e)
{
	unsigned int i;
	struct client *c;
	struct workspace *ws;
	struct monitor *mon;

	if (!locateclient(&ws, &c, NULL, e->xbutton.window, NULL) ||
			!locatemon(&mon, NULL, ws)) {
		return;
	}
	if (mon != selmon) {
		viewmon(mon);
	}
	push(ws, c);
	updatefocus();

	for (i = 0; i < LENGTH(buttons); i++) {
		if (buttons[i].mod == e->xbutton.state &&
				buttons[i].button == e->xbutton.button && buttons[i].func) {
			buttons[i].func(&buttons[i].arg);
		}
	}
}

bool
checksizehints(struct client *c, int unsigned *w, int unsigned *h)
{
	int u;
	bool change = false;

	/* don't respect size hints for tiled or fullscreen windows */
	if (!c->floating || c->fullscreen) {
		return *w != c->w || *h != c->h;
	}

	if (*w != c->w) {
		if (c->basew > 0 && c->incw > 0) {
			u = (*w-c->basew)/c->incw;
			*w = c->basew+u*c->incw;
		}
		if (c->minw > 0) {
			*w = MAX(*w, c->minw);
		}
		if (c->maxw > 0) {
			*w = MIN(*w, c->maxw);
		}
		if (*w != c->w) {
			change = true;
		}
	}
	if (*h != c->h) {
		if (c->baseh > 0 && c->inch > 0) {
			u = (*h-c->baseh)/c->inch;
			*h = c->baseh+u*c->inch;
		}
		if (c->minh > 0) {
			*h = MAX(*h, c->minh);
		}
		if (c->maxh > 0) {
			*h = MIN(*h, c->maxh);
		}
		if (*h != c->h) {
			change = true;
		}
	}
	return change;
}

void
cleanup(char **savefile)
{
	unsigned int i;
	struct monitor *mon;
	struct workspace *ws;
	struct client *c;

	/* disable WSM */
	if (wsm.active) {
		togglewsm(NULL);
	}

	/* remove scratchpad */
	if (pad) {
		free(pad);
		pad = NULL;
	}

	/* make monitors point nowhere (so all workspaces are removed) */
	for (i = 0; i < nmon; i++) {
		mon = monitors[i];
		if (!mon->selws->nc) {
			detachws(mon->selws);
			termws(mon->selws);
		} else {
			showws(NULL, mon->selws);
		}
		monitors[i]->selws = NULL;
	}

	/* if restarting, save the session before removing everything */
	if (restarting) {
		savesession(savefile);
	}

	/* remove workspaces and their clients */
	while (nws) {
		ws = workspaces[0];
		while (ws->nc) {
			c = ws->clients[0];
			detachclient(c);
			termclient(c);
		}
	}
	termws(wsm.target);

	/* remove monitors */
	while (nmon) {
		mon = monitors[0];
		detachmon(mon);
		termmon(mon);
	}

	/* remove layouts */
	for (i = 0; i < LENGTH(layouts); i++) {
		if (layouts[i].icon_bitfield) {
			termlayout(&layouts[i]);
		}
	}

	/* remove X resources */
	if (dc.font.xfontset) {
		XFreeFontSet(kwm.dpy, dc.font.xfontset);
	} else {
		XFreeFont(kwm.dpy, dc.font.xfontstruct);
	}
	XFreeGC(kwm.dpy, dc.gc);
	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, root);
	XFreeCursor(kwm.dpy, cursor[CurNormal]);
	XFreeCursor(kwm.dpy, cursor[CurResize]);
	XFreeCursor(kwm.dpy, cursor[CurMove]);
	XSetInputFocus(kwm.dpy, PointerRoot, RevertToPointerRoot, CurrentTime);
}

void
clientmessage(XEvent *e)
{
	DEBUG("clientmessage(%lu)", e->xclient.window);

	struct client *c;
	XClientMessageEvent *ev = &e->xclient;

	if (!locateclient(NULL, &c, NULL, ev->window, NULL)) {
		return;
	}

	if (ev->message_type == netatom[NetWMState]) {
		if (ev->data.l[1] == netatom[NetWMStateFullscreen] ||
				ev->data.l[2] == netatom[NetWMStateFullscreen]) {
			setfullscreen(c, ev->data.l[0] == 1 || /* _NET_WM_STATE_ADD */
					(ev->data.l[0] == 2 /* _NET_WM_STATE_TOGGLE */ &&
					 !c->fullscreen));
		}
	}
}

void
configurenotify(XEvent *e)
{
	//debug("configurenotify(%lu)", e->xconfigure.window);
	if (e->xconfigure.window == root) {
		updategeom();
	}
}

void
configurerequest(XEvent *e)
{
	DEBUG("configurerequest(%lu)", e->xconfigurerequest.window);

	struct client *c = NULL;
	XWindowChanges wc;
	XConfigureEvent cev;
	XConfigureRequestEvent *ev = &e->xconfigurerequest;

	/* forward configuration if not managed (or if we don't force the size) */
	if ((pad && ev->window == pad->win) ||
			!locateclient(NULL, &c, NULL, ev->window, NULL) || c->fullscreen ||
			c->floating) {
		wc = (XWindowChanges) {
			.x = ev->x,
			.y = ev->y,
			.width = ev->width,
			.height = ev->height,
			.border_width = ev->border_width,
			.sibling = ev->above,
			.stack_mode = ev->detail
		};
		XConfigureWindow(kwm.dpy, ev->window, ev->value_mask, &wc);
		return;
	}

	/* force size with XSendEvent() instead of ordinary XConfigureWindow() */
	cev = (XConfigureEvent) {
		.type = ConfigureNotify,
		.display = kwm.dpy,
		.event = c->win,
		.window = c->win,
		.x = c->x,
		.y = c->y,
		.width = c->w,
		.height = c->h,
		.border_width = c->border,
		.above = None,
		.override_redirect = False,
	};
	XSendEvent(kwm.dpy, c->win, false, StructureNotifyMask, (XEvent *) &cev);
}

void
detachclient(struct client *c)
{
	unsigned int i;
	struct workspace *ws;
	struct monitor *mon;

	if (!locateclient(&ws, &c, &i, c->win, NULL)) {
		WARN("attempt to detach non-existent client");
		return;
	}

	/* remove from stack */
	pop(ws, c);

	/* remove from list */
	ws->nc--;
	for (; i < ws->nc; i++) {
		ws->clients[i] = ws->clients[i+1];
	}
	ws->clients = realloc(ws->clients, ws->nc*sizeof(struct client *));
	if (!ws->clients && ws->nc) {
		FATAL("could not allocate %zu bytes for client list",
		      ws->nc*sizeof(struct client *));
		exit(EXIT_FAILURE);
	}

	/* remove workspace if last client and not focused */
	if (!ws->nc && !locatemon(&mon, NULL, ws)) {
		detachws(ws);
		termws(ws);
	} else {
		ws->dirty = true;
	}
}

void
detachmon(struct monitor *mon)
{
	unsigned int i;

	for (i = 0; i < nmon; i++) {
		if (monitors[i] == mon) {
			nmon--;
			for (; i < nmon; i++) {
				monitors[i] = monitors[i+1];
			}
			monitors = realloc(monitors, nmon*sizeof(struct monitor *));
			if (!monitors && nmon) {
				FATAL("could not allocate %zu bytes for monitor list",
				      nmon*sizeof(struct monitor *));
				exit(EXIT_FAILURE);
			}
			return;
		}
	}
	WARN("attempt to detach non-existing monitor");
}

void
detachws(struct workspace *ws)
{
	unsigned int i;

	if (!locatews(&ws, &i, ws->x, ws->y, NULL)) {
		WARN("attempt to detach non-existent workspace");
		return;
	}

	nws--;
	for (; i < nws; i++) {
		workspaces[i] = workspaces[i+1];
	}
	workspaces = realloc(workspaces, nws*sizeof(struct workspace *));
	if (!workspaces && nws) {
		FATAL("could not allocate %zu bytes for workspace list",
		      nws*sizeof(struct workspace *));
		exit(EXIT_FAILURE);
	}
}

void
dmenu(union argument const *arg)
{
	int in[2], out[2], j, i, len;
	char const *cmd[LENGTH(dmenuargs)+3], *cname;

	/* assemble dmenu command */
	cmd[0] = arg->i == DMenuSpawn ? "dmenu_run" : "dmenu";
	for (i = 0; i < LENGTH(dmenuargs); i++) {
		cmd[i+1] = dmenuargs[i];
	}
	cmd[i] = "-p";
	cmd[i+1] = dmenuprompt[arg->i];
	cmd[i+2] = NULL;

	/* create pipes */
	if (arg->i != DMenuSpawn && (pipe(in) < 0 || pipe(out) < 0)) {
		ERROR("could not create pipes for dmenu");
		return;
	}

	/* child */
	if (fork() == 0) {
		if (arg->i != DMenuSpawn) {
			close(in[1]);
			close(out[0]);
			dup2(in[0], STDIN_FILENO);
			dup2(out[1], STDOUT_FILENO);
			close(in[0]);
			close(out[1]);
		}
		execvp(cmd[0], (char *const *) cmd);
		ERROR("failed to execute %s: %s", cmd[0], strerror(errno));
		exit(EXIT_FAILURE);
	}

	/* parent */
	if (arg->i == DMenuSpawn) {
		return;
	}
	close(in[0]);
	close(out[1]);
	if (arg->i != DMenuRename) {
		if (arg->i == DMenuClients) {
			for (j = 0; j < nws; j++) {
				for (i = 0; i < workspaces[j]->nc; i++) {
					cname = workspaces[j]->clients[i]->name;
					len = strlen(cname);
					if (!len) {
						continue;
					}
					write(in[1], cname, len);
					write(in[1], "\n", 1);
				}
			}
		} else {
			for (i = 0; i < nws; i++) {
				write(in[1], workspaces[i]->name, strlen(workspaces[i]->name));
				write(in[1], "\n", 1);
			}
		}
	}
	close(in[1]);
	dmenu_state = arg->i;
	dmenu_out = out[0];
}

void
dmenueval(void)
{
	int ret;
	char buf[256];
	struct monitor *mon=NULL;
	struct workspace *ws;
	struct client *c;

	ret = read(dmenu_out, buf, 256);
	if (ret < 0) {
		ERROR("failed to read from dmenu file descriptor");
		return;
	}
	if (ret > 0) {
		buf[ret-1] = 0;
		if (dmenu_state == DMenuClients) {
			locateclient(&ws, &c, NULL, 0, buf);
			if (ws != selmon->selws) {
				setws(selmon, ws->x, ws->y);
			}
			push(ws, c);
			updatefocus();
		} else if (dmenu_state == DMenuRename) {
			renamews(selmon->selws, buf);
			detachws(selmon->selws);
			attachws(selmon->selws);
		} else if (locatews(&ws, NULL, 0, 0, buf)) {
			if (dmenu_state == DMenuSend || dmenu_state == DMenuSendFollow) {
				if (selmon->selws->nc) {
					c = selmon->selws->selcli;
					detachclient(c);
					arrange(selmon);
					attachclient(ws, c);
					if (locatemon(&mon, NULL, ws)) {
						arrange(mon);
					} else {
						showclient(NULL, c);
					}
					updatefocus();
				}
			}
			if (dmenu_state == DMenuSendFollow || dmenu_state == DMenuView) {
				setws(selmon, ws->x, ws->y);
			}
		}
	}
	close(dmenu_out);
	dmenu_state = DMenuInactive;
}

void
enternotify(XEvent *e)
{
	DEBUG("enternotify(%lu)", e->xcrossing.window);

	unsigned int pos;
	struct workspace *ws;
	struct monitor *mon;
	struct client *c;

	if (pad && e->xcrossing.window == pad->win) {
		viewmon(pad_mon);
	} else {
		if (!locateclient(&ws, &c, &pos, e->xcrossing.window, NULL)) {
			WARN("attempt to enter unhandled/invisible window %d",
			     e->xcrossing.window);
			return;
		}
		if (locatemon(&mon, NULL, ws) && mon != selmon) {
			viewmon(mon);
		}
		push(selmon->selws, c);
	}
	updatefocus();
}

void
expose(XEvent *e)
{
	DEBUG("expose(%lu)", e->xexpose.window);

	unsigned int i;

	/* WSM boxes */
	for (i = 0; i < nws; i++) {
		if (e->xexpose.window == workspaces[i]->wsmbox) {
			renderwsmbox(workspaces[i]);
		}
	}
	if (e->xexpose.window == wsm.target->wsmbox) {
		renderwsmbox(wsm.target);
	}
}

/* fix for VTE terminals that tend to steal the focus */
void
focusin(XEvent *e)
{
	Window win = e->xfocus.window;

	if (win == root) {
		return;
	}
	if (!selmon->selws->nc || win != selmon->selws->selcli->win) {
		WARN("focusin on unfocused window %d (focus is on %d)",
				e->xfocus.window,
				selmon->selws->nc ? selmon->selws->selcli->win : -1);
		updatefocus();
	}
}

int
gettiled(struct client ***tiled, struct monitor *mon)
{
	unsigned int i, n;
	struct client *c;

	*tiled = calloc(mon->selws->nc, sizeof(struct client *));
	if (!*tiled) {
		FATAL("could not allocate %zu bytes for client list",
		      mon->selws->nc*sizeof(struct client *));
		exit(EXIT_FAILURE);
	}
	for (n = i = 0; i < mon->selws->nc; i++) {
		c = mon->selws->clients[i];
		if (!c->floating && !c->fullscreen) {
			(*tiled)[n++] = mon->selws->clients[i];
		} else {
		}
	}
	*tiled = realloc(*tiled, n*sizeof(struct client *));
	if (!tiled && n) {
		FATAL("could not allocate %zu bytes for client list",
		      n*sizeof(struct client *));
		exit(EXIT_FAILURE);
	}
	return n;
}

void
grabbuttons(struct client *c, bool focused)
{
	unsigned int i;

	XUngrabButton(kwm.dpy, AnyButton, AnyModifier, c->win);
	if (focused) {
		for (i = 0; i < LENGTH(buttons); i++) {
			XGrabButton(kwm.dpy, buttons[i].button, buttons[i].mod, c->win, false,
					BUTTONMASK, GrabModeAsync, GrabModeAsync, None, None);
		}
	} else {
		XGrabButton(kwm.dpy, AnyButton, AnyModifier, c->win, false, BUTTONMASK,
				GrabModeAsync, GrabModeAsync, None, None);
	}
}

void
grabkeys(void)
{
	unsigned int i;

	XUngrabKey(kwm.dpy, AnyKey, AnyModifier, root);
	if (wsm.active) {
		XGrabKeyboard(kwm.dpy, wsm.target->wsmbox, true, GrabModeAsync,
				GrabModeAsync, CurrentTime);
		for (i = 0; i < LENGTH(wsmkeys); i++) {
			XGrabKey(kwm.dpy, XKeysymToKeycode(kwm.dpy, wsmkeys[i].key), wsmkeys[i].mod,
					root, true, GrabModeAsync, GrabModeAsync);
		}
	} else {
		for (i = 0; i < LENGTH(keys); i++) {
			XGrabKey(kwm.dpy, XKeysymToKeycode(kwm.dpy, keys[i].key), keys[i].mod, root,
					true, GrabModeAsync, GrabModeAsync);
		}
	}
}

Pixmap
initpixmap(long const *bitfield, long const fg, long const bg)
{
	int x, y;
	long w = bitfield[0];
	long h = bitfield[1];
	Pixmap icon;

	icon = XCreatePixmap(kwm.dpy, root, w, h, dc.sd);
	XSetForeground(kwm.dpy, dc.gc, bg);
	XFillRectangle(kwm.dpy, icon, dc.gc, 0, 0, w, h);
	XSetForeground(kwm.dpy, dc.gc, fg);
	for (y = 0; y < h; y++) {
		for (x = 0; x < w; x++) {
			if ((bitfield[y+2]>>(w-x-1))&1) {
				XDrawPoint(kwm.dpy, icon, dc.gc, x, y);
			}
		}
	}
	return icon;
}

struct monitor *
initmon(void)
{
	unsigned int wsx;
	struct monitor *mon;
	struct workspace *ws;

	mon = malloc(sizeof(struct monitor));
	if (!mon) {
		FATAL("could not allocate %zu bytes for monitor",
		      sizeof(struct monitor));
		exit(EXIT_FAILURE);
	}

	/* assign workspace */
	for (wsx = 0;; wsx++) {
		if (!locatews(&ws, NULL, wsx, 0, NULL)) {
			mon->selws = initws(wsx, 0);
			attachws(mon->selws);
			break;
		} else if (!locatemon(NULL, NULL, ws)) {
			mon->selws = ws;
			break;
		}
	}
	return mon;
}

struct workspace *
initws(int x, int y)
{
	struct workspace *ws;

	if (locatews(&ws, NULL, x, y, NULL)) {
		return ws;
	}
	ws = malloc(sizeof(struct workspace));
	if (!ws) {
		FATAL("could not allocate %zu bytes for workspace",
		      sizeof(struct workspace));
		exit(EXIT_FAILURE);
	}
	ws->clients = ws->stack = NULL;
	ws->nc = ws->ns = 0;
	ws->x = x;
	ws->y = y;
	ws->mfact = MFACT;
	ws->nmaster = NMASTER;
	ws->ilayout = 0;
	ws->dirty = false;
	renamews(ws, NULL);
	if (wsm.active) {
		initwsmbox(ws);
	} else {
		ws->wsmbox = ws->wsmpm = 0;
	}
	return ws;
}

void
initwsmbox(struct workspace *ws)
{
	/* pixmap */
	ws->wsmpm = XCreatePixmap(kwm.dpy, root, WSMBOXWIDTH, WSMBOXHEIGHT, dc.sd);
	updatewsmpixmap(ws);

	/* box */
	ws->wsmbox = XCreateWindow(kwm.dpy, root, -WSMBOXWIDTH-2*WSMBORDERWIDTH, 0,
			WSMBOXWIDTH, WSMBOXHEIGHT, 0, dc.sd, CopyFromParent,
			DefaultVisual(kwm.dpy, screen),
			CWOverrideRedirect|CWBackPixmap|CWEventMask, &wsm.wa);
	XMapRaised(kwm.dpy, ws->wsmbox);
	XSetWindowBorderWidth(kwm.dpy, ws->wsmbox, WSMBORDERWIDTH);
	XSetWindowBorder(kwm.dpy, ws->wsmbox, ws == selmon->selws ? WSMCBORDERSEL
			: ws == wsm.target ? WSMCBORDERTARGET : WSMCBORDERNORM);

	/* initial render */
	renderwsmbox(ws);
}

void
keypress(XEvent *e)
{
	//debug("keypress()");

	unsigned int i;
	KeySym keysym = XLookupKeysym(&e->xkey, 0);

	if (wsm.active) {
		for (i = 0; i < LENGTH(wsmkeys); i++) {
			if (e->xkey.state == wsmkeys[i].mod && keysym == wsmkeys[i].key &&
					wsmkeys[i].func) {
				wsmkeys[i].func(&wsmkeys[i].arg);
				updatewsm();
				return;
			}
		}
	} else {
		for (i = 0; i < LENGTH(keys); i++) {
			if (e->xkey.state == keys[i].mod && keysym == keys[i].key &&
					keys[i].func) {
				keys[i].func(&keys[i].arg);
				return;
			}
		}
	}
}

void
keyrelease(XEvent *e)
{
	//debug("keyrelease()");
}

void
killclient(union argument const *arg)
{
	struct client *c;
	(void) arg;

	/* nothing to kill */
	if (!selmon->selws->nc) {
		return;
	}

	/* try to kill the client via the WM_DELETE_WINDOW atom */
	c = selmon->selws->selcli;
	if (sendevent(c, wmatom[WMDeleteWindow])) {
		return;
	}

	/* otherwise massacre the client */
	XGrabServer(kwm.dpy);
	XSetCloseDownMode(kwm.dpy, DestroyAll);
	XKillClient(kwm.dpy, selmon->selws->selcli->win);
	XSync(kwm.dpy, false);
	XUngrabServer(kwm.dpy);
}

bool
locateclient(struct workspace **ws, struct client **c, unsigned int *pos, Window w, char const *name)
{
	unsigned int i, j;

	if (name)
		DEBUG("searching for client '%s'", name);
	for (j = 0; j < nws; j++) {
		for (i = 0; i < workspaces[j]->nc; i++) {
			if ((name && !strcmp(workspaces[j]->clients[i]->name, name)) ||
					workspaces[j]->clients[i]->win == w) {
				if (ws) *ws = workspaces[j];
				if (c) *c = workspaces[j]->clients[i];
				if (pos) *pos = i;
				return true;
			}
		}
	}
	if (name)
		DEBUG("=> not found: '%s'", name);
	return false;
}

bool
locatemon(struct monitor **mon, unsigned int *pos, struct workspace const *ws)
{
	unsigned int i;

	if (!ws) {
		return false;
	}
	for (i = 0; i < nmon; i++) {
		if (monitors[i]->selws == ws) {
			if (mon) *mon = monitors[i];
			if (pos) *pos = i;
			return true;;
		}
	}
	return false;
}

struct monitor *
locaterect(int x, int y, int w, int h)
{
	struct monitor *mon = selmon;
	unsigned int i;
	int a, area=0;

	for (i = 0; i < nmon; i++) {
		if ((a = INTERSECT(monitors[i], x, y, w, h)) > area) {
			area = a;
			mon = monitors[i];
		}
	}
	return mon;
}

bool
locatews(struct workspace **ws, unsigned int *pos, int x, int y, char const *name)
{
	unsigned int i;

	if (name) {
		for (i = 0; i < nws; i++) {
			if (!strcmp(name, workspaces[i]->name)) {
				if (ws) *ws = workspaces[i];
				if (pos) *pos = i;
				return true;
			}
		}
	} else {
		for (i = 0; i < nws; i++) {
			if (workspaces[i]->x == x && workspaces[i]->y == y) {
				if (ws) *ws = workspaces[i];
				if (pos) *pos = i;
				return true;
			}
		}
	}
	return false;
}

void
maprequest(XEvent *e)
{
	DEBUG("maprequest(%lu)", e->xmaprequest.window);

	struct client *c;
	Window win = e->xmaprequest.window;

	/* in case a window gets remapped */
	if (locateclient(NULL, &c, NULL, win, NULL)) {
		detachclient(c);
		termclient(c);
	}
	if (!(c = client_init(win, false))) {
		return;
	}

	/* display */
	attachclient(selmon->selws, c);
	arrange(selmon);
	XMapWindow(kwm.dpy, c->win);

	/* update client information (fullscreen/dialog/transient) and focus */
	updateclient(c);
	updatetransient(c);
	updatefocus();
}

void
mappingnotify(XEvent *e)
{
	DEBUG("mappingnotify(%lu)", e->xmapping.window);
	/* TODO */
}

void
movemouse(union argument const *arg)
{
	struct monitor *mon;
	XEvent ev;
	Window dummy;
	struct client *c = selmon->selws->selcli;
	int cx=c->x, cy=c->y, x, y, i;
	unsigned int ui;
	(void) arg;

	DEBUG("movemouse(%lu)", c->win);

	/* don't move fullscreen client */
	if (c->fullscreen) {
		return;
	}

	/* grab the pointer and change the cursor appearance */
	if (XGrabPointer(kwm.dpy, root, true, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, cursor[CurMove], CurrentTime) != GrabSuccess) {
		WARN("XGrabPointer() failed");
		return;
	}

	/* get initial pointer position */
	if (!XQueryPointer(kwm.dpy, root, &dummy, &dummy, &x, &y, &i, &i, &ui)) {
		WARN("XQueryPointer() failed");
		return;
	}

	/* handle motions */
	do {
		XMaskEvent(kwm.dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		switch (ev.type) {
			case ConfigureRequest:
			case Expose:
			case MapRequest:
				handle[ev.type](&ev);
				break;
			case MotionNotify:
				if (!c->floating) {
					togglefloat(NULL);
				}
				cx = cx+(ev.xmotion.x-x);
				cy = cy+(ev.xmotion.y-y);
				x = ev.xmotion.x;
				y = ev.xmotion.y;
				client_move(selmon, c, cx, cy);
				break;
		}
	} while (ev.type != ButtonRelease);

	/* check if it has been moved to another monitor */
	mon = locaterect(selmon->x+c->x, selmon->y+c->y, c->w, c->h);
	if (mon != selmon) {
		c->x = selmon->x+c->x-mon->x;
		c->y = selmon->y+c->y-mon->y;
		detachclient(c);
		attachclient(mon->selws, c);
		viewmon(mon);
		updatefocus();
	}

	XUngrabPointer(kwm.dpy, CurrentTime);
}

void
moveresizeclient(struct monitor *mon, struct client *c, int x, int y, int unsigned w, int unsigned h)
{
	client_move(mon, c, x, y);
	resizeclient(c, w, h);
}

void
pop(struct workspace *ws, struct client *c)
{
	unsigned int i;

	if (!c) {
		WARN("attempt to pop NULL client");
		return;
	}

	for (i = 0; i < ws->ns; i++) {
		if (ws->stack[i] == c) {
			ws->ns--;
			for (; i < ws->ns; i++) {
				ws->stack[i] = ws->stack[i+1];
			}
			ws->stack = realloc(ws->stack, ws->ns*sizeof(struct client *));
			if (ws->ns && !ws->stack) {
				FATAL("could not allocate %zu bytes for stack",
				      ws->ns*sizeof(struct client*));
				exit(EXIT_FAILURE);
			}
			break;
		}
	}
	ws->selcli = ws->ns ? ws->stack[ws->ns-1] : NULL;
}

void
print(FILE *f, enum log_level level, char const *format, ...)
{
	va_list args;
	time_t rawtime;
	struct tm *date;
	char const *col;

	if (level > log_level)
		return;

	/* application name & timestamp */
	time(&rawtime);
	date = localtime(&rawtime);
	(void) fprintf(f, APPNAME" [%04d-%02d-%02d|%02d:%02d:%02d] ",
	               date->tm_year+1900, date->tm_mon, date->tm_mday,
	               date->tm_hour, date->tm_min, date->tm_sec);

	/* log level */
	switch (level) {
	case LOG_FATAL: col = "\033[31mFATAL\033[0m "; break;
	case LOG_ERROR: col = "\033[31mERROR\033[0m "; break;
	case LOG_WARN : col = "\033[33mWARN\033[0m " ; break;
	default       : col = "";
	}
	(void) fprintf(f, "%s ", col);

	/* message */
	va_start(args, format);
	(void) vfprintf(f, format, args);
	va_end(args);

	(void) fprintf(f, "\n");
	(void) fflush(f);
}

void
propertynotify(XEvent *e)
{
	DEBUG("propertynotify(%lu)", e->xproperty.window);

	XPropertyEvent *ev;
	struct client *c;

	ev = &e->xproperty;
	if (!locateclient(NULL, &c, NULL, ev->window, NULL)) {
		return;
	}
	switch (ev->atom) {
		case XA_WM_TRANSIENT_FOR:
			DEBUG("\033[33mwindow %lu is transient!\033[0m", c->win);
			updatetransient(c);
			break;
		case XA_WM_NORMAL_HINTS:
			client_updatesizehints(c);
			break;
		case XA_WM_HINTS:
			/* TODO urgent hint */
			break;
	}
	if (ev->atom == XA_WM_NAME || ev->atom == netatom[NetWMName]) {
		client_updatename(c);
	}
	if (ev->atom == netatom[NetWMWindowType]) {
		updateclient(c);
	}
}

void
push(struct workspace *ws, struct client *c)
{
	if (!c) {
		WARN("attempt to push NULL client");
		return;
	}
	pop(ws, c);
	ws->stack = realloc(ws->stack, ++ws->ns*sizeof(struct client *));
	if (!ws->stack) {
		FATAL("could not allocate %zu bytes for stack",
		      ws->ns*sizeof(struct client *));
		exit(EXIT_FAILURE);
	}
	ws->selcli = ws->stack[ws->ns-1] = c;
}

void
quit(union argument const *arg)
{
	(void) arg;

	restarting = false;
	running = false;
}

void
reltoxy(int *x, int *y, struct workspace *ws, int direction)
{
	if (x) *x = ws->x;
	if (y) *y = ws->y;
	switch (direction) {
	case Left:  if (x) (*x)--; break;
	case Right: if (x) (*x)++; break;
	case Up:    if (y) (*y)--; break;
	case Down:  if (y) (*y)++; break;
	}
}

void
renamews(struct workspace *ws, char const *name)
{
	if (name == NULL || strlen(name) == 0) {
		sprintf(ws->name, "*%p", (void *) ws);
		return;
	}
	if (name[0] != '*' && !locatews(NULL, NULL, 0, 0, name)) {
		strncpy(ws->name, name, 256);
	}
	if (strcmp(name, "chat") == 0) { /* TODO rules list */
		/* TODO layout pointer instead of index */
		ws->ilayout = 4;
		arrange(selmon);
	}
}

void
renderwsmbox(struct workspace *ws)
{
	XCopyArea(kwm.dpy, ws->wsmpm, ws->wsmbox, dc.gc, 0,0, WSMBOXWIDTH, WSMBOXHEIGHT,
			0, 0);
}

void
resizeclient(struct client *c, int unsigned w, int unsigned h)
{
	if (checksizehints(c, &w, &h))
		XResizeWindow(kwm.dpy, c->win, c->w = MAX(w, 1), c->h = MAX(h, 1));
}

void
resizemouse(union argument const *arg)
{
	XEvent ev;
	struct client *c = selmon->selws->selcli;
	int x, y;
	unsigned int cw=c->w, ch=c->h;
	(void) arg;

	DEBUG("resizemouse(%lu)", c->win);

	/* don't resize fullscreen or dialog client */
	if (c->fullscreen || c->dialog) {
		WARN("attempt to resize fullscreen client");
		return;
	}

	/* grab the pointer and change the cursor appearance */
	if (XGrabPointer(kwm.dpy, root, false, MOUSEMASK, GrabModeAsync, GrabModeAsync,
			None, cursor[CurResize], CurrentTime) != GrabSuccess) {
		WARN("XGrabPointer() failed");
		return;
	}

	/* set pointer position to lower right */
	XWarpPointer(kwm.dpy, None, c->win, 0, 0, 0, 0,
			c->w+2*c->border-1, c->h+2*c->border-1);
	x = selmon->x+c->x+c->w+2*c->border-1;
	y = selmon->y+c->y+c->h+2*c->border-1;

	/* handle motions */
	do {
		XMaskEvent(kwm.dpy, MOUSEMASK|ExposureMask|SubstructureRedirectMask, &ev);
		//debug("\033[38;5;238m[resizemouse] run(): e.type=%d\033[0m", ev.type);
		switch (ev.type) {
		case ConfigureRequest:
		case Expose:
		case MapRequest:
			handle[ev.type](&ev);
			break;
		case MotionNotify:
			if (!c->floating) {
				togglefloat(NULL);
			}
			cw = cw+(ev.xmotion.x_root-x);
			ch = ch+(ev.xmotion.y_root-y);
			x = ev.xmotion.x_root;
			y = ev.xmotion.y_root;
			resizeclient(c, cw, ch);
			break;
		}
	} while (ev.type != ButtonRelease);

	XUngrabPointer(kwm.dpy, CurrentTime);
}

void
restart(union argument const *arg)
{
	(void) arg;

	restarting = true;
	running = false;
}

void
restoresession(char const *sessionfile)
{
	FILE *f;
	unsigned int i, j, _nc, _nws;
	Window win;
	int x, y;
	struct workspace *ws;
	struct client *c;
	char name[256];

	f = fopen(sessionfile, "r");
	if (!f) {
		NOTE("could not find session file %s", sessionfile);
		return;
	}

	fscanf(f, "%u\n", &_nws);
	for (i = 0; i < _nws; i++) {
		fscanf(f, "%d:%d:", &x, &y);
		ws=initws(x, y);
		fscanf(f, "%u:%u:%f:%d:%s\n", &ws->nmaster, &_nc, &ws->mfact,
				&ws->ilayout, name);
		renamews(ws, name);
		attachws(ws);
		for (j = 0; j < _nc; j++) {
			fscanf(f, "  %lu:", &win);
			c = client_init(win, true);
			if (!c) {
				fscanf(f, "[^\n]\n");
				continue;
			}
			attachclient(ws, c);
			fscanf(f, "%d:%d:%d:%d:%d:%d\n", &c->x, &c->y, &c->w, &c->h,
					(int *) &c->floating, (int *) &c->fullscreen);
			c->dirty = true;
		}
	}

	fclose(f);
	remove(sessionfile);
}

void
run(void)
{
	XEvent e;
	int ret;
	fd_set fds;
	struct timeval timeout = { .tv_usec = 0, .tv_sec = 0 };
	dmenu_out =

	running = true;
	while (running && !XNextEvent(kwm.dpy, &e)) {
		//debug("\033[38;5;238mrun(): e.type=%d\033[0m", e.type);
		if (handle[e.type]) {
			handle[e.type](&e);
		}
		if (dmenu_state != DMenuInactive) {
			FD_ZERO(&fds);
			FD_SET(dmenu_out, &fds);
			ret = select(FD_SETSIZE, &fds, NULL, NULL, &timeout);
			if (ret > 0) {
				dmenueval();
			}
		}
	}
}

void
savesession(char **sessionfile)
{
	FILE *f;
	unsigned int i, j;
	struct client *c;
	struct workspace *ws;

	*sessionfile = malloc(strlen(SESSIONFILE+1+(sizeof(int)<<1)));
	srand(time(NULL));
	sprintf(*sessionfile, "%s-%0*X", SESSIONFILE, (int) sizeof(int)<<1, rand());
	f = fopen(*sessionfile, "w");
	if (!f) {
		FATAL("could not open session file %s", *sessionfile);
		exit(EXIT_FAILURE);
	}

	fprintf(f, "%u\n", nws);
	NOTE("storing %u workspaces:", nws);
	for (i = 0; i < nws; i++) {
		ws = workspaces[i];
		NOTE("- storing workspace[%u] (%d,%d): '%s' (%u clients)",
				i, ws->x, ws->y, ws->name, ws->nc);
		fprintf(f, "%d:%d:%u:%u:%f:%d:%s\n", ws->x, ws->y, ws->nmaster, ws->nc,
				ws->mfact, ws->ilayout, ws->name);
		for (j = 0; j < ws->nc; j++) {
			c = ws->clients[j];
			NOTE("  storing client '%s' (%u)", c->name, c->win);
			fprintf(f, "  %lu:%d:%d:%d:%d:%d:%d\n",
					c->win, c->x, c->y, c->w, c->h, c->floating, c->fullscreen);
		}
	}

	fclose(f);
}

bool
sendevent(struct client *c, Atom atom)
{
	int n;
	Atom *supported;
	bool exists;
	XEvent ev;

	if (XGetWMProtocols(kwm.dpy, c->win, &supported, &n)) {
		while (!exists && n--) {
			exists = supported[n] == atom;
		}
		XFree(supported);
	}
	if (!exists) {
		return false;
	}
	ev.type = ClientMessage;
	ev.xclient.window = c->win;
	ev.xclient.message_type = wmatom[WMProtocols];
	ev.xclient.format = 32;
	ev.xclient.data.l[0] = atom;
	ev.xclient.data.l[1] = CurrentTime;
	XSendEvent(kwm.dpy, c->win, false, NoEventMask, &ev);
	return true;
}

void
sendfollowclient(union argument const *arg)
{
	if (!selmon->selws->nc) {
		return;
	}
	struct client *c = selmon->selws->selcli;
	detachclient(c);
	stepws(arg);
	attachclient(selmon->selws, c);
	arrange(selmon);
	updatefocus();
}

void
setclientmask(struct monitor *mon, bool set)
{
	int i;

	if (!mon) {
		return;
	}
	if (set) {
		for (i = 0; i < mon->selws->nc; i++) {
			XSelectInput(kwm.dpy, mon->selws->clients[i]->win, CLIENTMASK);
		}
	} else {
		for (i = 0; i < mon->selws->nc; i++) {
			XSelectInput(kwm.dpy, mon->selws->clients[i]->win, 0);
		}
	}
}

void
setfloating(struct client *c, bool floating)
{
	struct workspace *ws;
	struct monitor *mon;

	if (!locateclient(&ws, NULL, NULL, c->win, NULL)) {
		WARN("attempt to set non-existing client to floating");
		return;
	}

	if (floating) {
		XRaiseWindow(kwm.dpy, c->win);
	} else {
		XLowerWindow(kwm.dpy, c->win);
	}
	c->floating = floating;
	if (locatemon(&mon, NULL, ws)) {
		arrange(mon);
	}
	updatefocus();
}

void
setfullscreen(struct client *c, bool fullscreen)
{
	struct monitor *mon=NULL;
	struct workspace *ws;
	locateclient(&ws, NULL, NULL, c->win, NULL) && locatemon(&mon, NULL, ws);

	c->fullscreen = fullscreen;
	if (fullscreen) {
		c->oldx = c->x;
		c->oldy = c->y;
		c->oldw = c->w;
		c->oldh = c->h;
		XSetWindowBorderWidth(kwm.dpy, c->win, 0);
		moveresizeclient(mon, c, 0, 0, mon->w, mon->h);
		XRaiseWindow(kwm.dpy, c->win);
	} else {
		c->x = c->oldx;
		c->y = c->oldy;
		c->w = c->oldw;
		c->h = c->oldh;
		XSetWindowBorderWidth(kwm.dpy, c->win, c->border);
		if (!c->floating) {
			arrange(mon);
		}
	}
}

void
setmfact(union argument const *arg)
{
	(void) arg;

	selmon->selws->mfact = MAX(0.1, MIN(0.9, selmon->selws->mfact+arg->f));
	arrange(selmon);
}

void
setnmaster(union argument const *arg)
{
	(void) arg;

	if (!selmon->selws->nmaster && arg->i < 0) {
		return;
	}
	selmon->selws->nmaster = selmon->selws->nmaster+arg->i;
	arrange(selmon);
}

void
setpad(union argument const *arg)
{
	struct client *newpad = NULL;
	(void) arg;

	/* if pad has focus, unpad it */
	if (pad_mon == selmon) {
		attachclient(selmon->selws, pad);
		pad = NULL;
		pad_mon = NULL;
		arrange(selmon);
		updatefocus();
		return;
	}

	/* pad does not have focus; check if anything has focus */
	if (!selmon->selws->nc)
		return;

	/* set current focus as pad, remove old pad if necessary */
	newpad = selmon->selws->selcli;
	detachclient(newpad);
	if (pad) {
		attachclient(selmon->selws, pad);
	}
	pad = newpad;
	arrange(selmon);
	togglepad(NULL);
}

void
setup(char const *sessionfile)
{
	XSetWindowAttributes wa;

	/* low: errors, zombies, locale */
	xerrorxlib = XSetErrorHandler(xerror);
	sigchld(0);
	if (!setlocale(LC_ALL, "") || !XSupportsLocale()) {
		FATAL("could not set locale");
		exit(EXIT_FAILURE);
	}

	/* basic: root window, graphic context, atoms */
	screen = DefaultScreen(kwm.dpy);
	root = RootWindow(kwm.dpy, screen);
	dc.gc = XCreateGC(kwm.dpy, root, 0, NULL);
	dc.sd = DefaultDepth(kwm.dpy, screen);
	setupatoms();

	/* events */
	wa.event_mask = SubstructureNotifyMask|SubstructureRedirectMask|
			KeyPressMask|StructureNotifyMask|ButtonPressMask|PointerMotionMask|
			FocusChangeMask|PropertyChangeMask;
	XChangeWindowAttributes(kwm.dpy, root, CWEventMask, &wa);

	/* input: mouse, keyboard */
	cursor[CurNormal] = XCreateFontCursor(kwm.dpy, XC_left_ptr);
	cursor[CurResize] = XCreateFontCursor(kwm.dpy, XC_sizing);
	cursor[CurMove] = XCreateFontCursor(kwm.dpy, XC_fleur);
	wa.cursor = cursor[CurNormal];
	XChangeWindowAttributes(kwm.dpy, root, CWCursor, &wa);
	grabkeys();

	/* layouts, fonts, workspace map, scratchpad, dmenu */
	setuplayouts();
	setupfont();
	setupwsm();
	pad = NULL;
	pad_mon = NULL;
	dmenu_state = DMenuInactive;

	/* session */
	setupsession(sessionfile);
}

void
setupatoms(void)
{
	wmatom[WMProtocols] = XInternAtom(kwm.dpy, "WM_PROTOCOLS", false);
	wmatom[WMDeleteWindow] = XInternAtom(kwm.dpy, "WM_DELETE_WINDOW", false);
	wmatom[WMState] = XInternAtom(kwm.dpy, "WM_STATE", false);
	wmatom[WMTakeFocus] = XInternAtom(kwm.dpy, "WM_TAKE_FOCUS", false);
	netatom[NetActiveWindow] = XInternAtom(kwm.dpy, "_NET_ACTIVE_WINDOW", false);
	netatom[NetSupported] = XInternAtom(kwm.dpy, "_NET_SUPPORTED", false);
	netatom[NetWMName] = XInternAtom(kwm.dpy, "_NET_WM_NAME", false);
	netatom[NetWMState] = XInternAtom(kwm.dpy, "_NET_WM_STATE", false);
	netatom[NetWMStateFullscreen] = XInternAtom(kwm.dpy, "_NET_WM_STATE_FULLSCREEN", false);
	netatom[NetWMWindowType] = XInternAtom(kwm.dpy, "_NET_WM_WINDOW_TYPE", false);
	netatom[NetWMWindowTypeDialog] = XInternAtom(kwm.dpy, "_NET_WM_WINDOW_TYPE_DIALOG", false);
}

void
setupfont(void)
{
	XFontStruct **xfonts;
	char **xfontnames;
	char *def, **missing;
	int n;

	dc.font.xfontset = XCreateFontSet(kwm.dpy, FONTSTR, &missing, &n, &def);
	if (missing) {
		while (n--) {
			WARN("missing fontset: %s", missing[n]);
		}
		XFreeStringList(missing);
	}

	/* if fontset load is successful, get information; otherwise dummy font */
	if (dc.font.xfontset) {
		dc.font.ascent = dc.font.descent = 0;
		n = XFontsOfFontSet(dc.font.xfontset, &xfonts, &xfontnames);
		while (n--) {
			dc.font.ascent = MAX(dc.font.ascent, xfonts[n]->ascent);
			dc.font.descent = MAX(dc.font.descent, xfonts[n]->descent);
		}
	} else {
		dc.font.xfontstruct = XLoadQueryFont(kwm.dpy, FONTSTR);
		if (!dc.font.xfontstruct) {
			FATAL("cannot load font '%s'", FONTSTR);
			exit(EXIT_FAILURE);
		}
		dc.font.ascent = dc.font.xfontstruct->ascent;
		dc.font.descent = dc.font.xfontstruct->descent;
	}
	dc.font.height = dc.font.ascent + dc.font.descent;
}

void
setuplayouts()
{
	unsigned int i, len=LENGTH(layouts);

	for (nlayouts = 0; nlayouts < len && layouts[nlayouts].func != NULL &&
			layouts[nlayouts].icon_bitfield != NULL; nlayouts++);
	for (i = 0; i < len; i++) {
		if (layouts[i].icon_bitfield == NULL || layouts[i].func == NULL)
			continue;
		layouts[i].icon_norm = initpixmap(layouts[i].icon_bitfield, CNORM, CBGNORM);
		layouts[i].icon_sel = initpixmap(layouts[i].icon_bitfield, CBORDERSEL, CBGNORM);
		layouts[i].w = layouts[i].icon_bitfield[0];
		layouts[i].h = layouts[i].icon_bitfield[1];
	}
}

void
setupsession(char const *sessionfile)
{
	unsigned int i, j, nwins;
	Window p, r, *wins = NULL;
	struct client *c;

	/* check for an old session, otherwise setup initial workspace */
	restoresession(sessionfile);
	if (!workspaces) {
		attachws(initws(0, 0));
	}

	/* scan existing windows */
	if (!XQueryTree(kwm.dpy, root, &r, &p, &wins, &nwins)) {
		WARN("XQueryTree() failed");
		return;
	}

	/* create clients if they do not already exist */
	for (i = 0; i < nwins; i++) {
		if (!locateclient(NULL, &c, NULL, wins[i], NULL)) {
			c = client_init(wins[i], true);
			if (c) {
				WARN("scanned unhandled window %lu", c->win);
				attachclient(workspaces[0], c);
			}
		} else {
			c->dirty = false;
		}
	}

	/* delete clients that do not have any windows assigned to them */
	for (i = 0; i < nws; i++) {
		for (j = 0; j < workspaces[i]->nc; j++) {
			c = workspaces[i]->clients[j];
			if (c->dirty) {
				WARN("restored client for non-existing window %lu", c->win);
				detachclient(c);
				termclient(c);
			}
		}
	}

	/* setup monitors */
	updategeom();
}

void
setupwsm(void)
{
	/* WSM data */
	wsm.active = false;

	/* WSM window informations */
	wsm.wa.override_redirect = true;
	wsm.wa.background_pixmap = ParentRelative;
	wsm.wa.event_mask = ExposureMask;

	/* dummy workspace for target box */
	wsm.target = initws(0, 0);
	wsm.target->name[0] = 0;
}

void
setws(struct monitor *mon, int x, int y)
{
	struct workspace *srcws, *dstws=NULL;
	struct monitor *othermon=NULL;

	/* get source and destination workspaces */
	srcws = mon->selws;
	if (locatews(&dstws, NULL, x, y, NULL) && srcws == dstws) {
		return;
	}

	/* exchange monitors on collision */
	if (dstws && locatemon(&othermon, NULL, dstws) && othermon != mon) {
		srcws->dirty = dstws->dirty = true; /* for different screen sizes */
		showws(othermon, srcws);
		showws(mon, dstws);
		updatefocus();
		return;
	}

	/* destination workspace */
	if (!dstws) {
		dstws = initws(x, y);
		attachws(dstws);
	}
	showws(mon, dstws);

	/* source workspace */
	if (srcws->nc) {
		showws(NULL, srcws);
	} else {
		detachws(srcws);
		termws(srcws);
	}

	updatefocus();
}

void
shiftclient(union argument const *arg)
{
	unsigned int pos;
	(void) arg;

	if (selmon->selws->ns < 2) {
		return;
	}
	if (!locateclient(NULL, NULL, &pos, selmon->selws->selcli->win, NULL)) {
		WARN("attempt to shift non-existent window %d",
				selmon->selws->selcli->win);
	}
	selmon->selws->clients[pos] = selmon->selws->clients[
			(pos+selmon->selws->nc+arg->i)%selmon->selws->nc];
	selmon->selws->clients[(pos+selmon->selws->nc+arg->i)%selmon->selws->nc] =
			selmon->selws->selcli;

	arrange(selmon);
}

void
shiftws(union argument const *arg)
{
	struct workspace *src=NULL, *dst=NULL;
	struct workspace *ref = wsm.active ? wsm.target : selmon->selws;
	int dx, dy;

	reltoxy(&dx, &dy, ref, arg->i);
	locatews(&src, NULL, ref->x, ref->y, NULL);
	locatews(&dst, NULL, dx, dy, NULL);

	if (dst) {
		dst->x = ref->x;
		dst->y = ref->y;
	}
	if (src) {
		src->x = dx;
		src->y = dy;
	}
	if (wsm.active) {
		wsm.target->x = dx;
		wsm.target->y = dy;
	}
}

void
showclient(struct monitor *mon, struct client *c)
{
	if (mon) {
		XMoveWindow(kwm.dpy, c->win, mon->x + c->x, mon->y + c->y);
	} else {
		XMoveWindow(kwm.dpy, c->win, -c->w-2*c->border, c->y);
	}
}

void
showws(struct monitor *mon, struct workspace *ws)
{
	unsigned int i;

	if (mon) {
		mon->selws = ws;
	} else if (!ws->nc) {
		detachws(ws);
		termws(ws);
		return;
	}
	if (mon && (ws->dirty || mon != ws->mon)) {
		arrange(mon);
	} else {
		setclientmask(mon, false);
		for (i = 0; i < ws->nc; i++) {
			showclient(mon, ws->clients[i]);
		}
		setclientmask(mon, true);
	}
}

void
sigchld(int unused)
{
	if (signal(SIGCHLD, sigchld) == SIG_ERR) {
		FATAL("could not install SIGCHLD handler");
		exit(EXIT_FAILURE);
	}
	/* pid -1 makes it equivalent to wait() (wait for all children);
	 * here we just add WNOHANG */
	while (0 < waitpid(-1, NULL, WNOHANG));
}

void
spawn(union argument const *arg)
{
	pid_t pid = fork();
	if (pid == 0) {
		execvp(((char const **)arg->v)[0], (char **)arg->v);
		ERROR("execvp(%s) failed: %s",
		      ((char const **)arg->v)[0], strerror(errno));
		exit(EXIT_FAILURE);
	}
	if (pid < 0) {
		WARN("fork() failed with code %d", pid);
	}
}

void
stepfocus(union argument const *arg)
{
	unsigned int pos;

	if (!selmon->selws->nc) {
		return;
	}
	if (!locateclient(NULL, NULL, &pos, selmon->selws->selcli->win, NULL)) {
		WARN("attempt to step from non-existing client");
		return;
	}
	push(selmon->selws, selmon->selws->clients[
			(pos+selmon->selws->nc+arg->i)%selmon->selws->nc]);
	updatefocus();
	if (selmon->selws->selcli->floating) {
		XRaiseWindow(kwm.dpy, selmon->selws->selcli->win);
	}
}

void
steplayout(union argument const *arg)
{
	selmon->selws->ilayout = (selmon->selws->ilayout+nlayouts+arg->i)%nlayouts;
	selmon->selws->mfact = MFACT;
	arrange(selmon);
}

void
stepmon(union argument const *arg)
{
	struct monitor *oldmon;
	unsigned int pos;
	(void) arg; /* TODO use arg->i */

	if (!locatemon(&oldmon, &pos, selmon->selws)) {
		WARN("attempt to step from non-existing monitor");
		return;
	}
	oldmon = selmon;
	viewmon(monitors[(pos+nmon+1)%nmon]);
	updatefocus();
}

void
stepws(union argument const *arg)
{
	int x, y;
	reltoxy(&x, &y, selmon->selws, arg->i);
	setws(selmon, x, y);
}

void
stepwsmbox(union argument const *arg)
{
	struct workspace *ws;

	wsm.target->name[0] = 0;
	reltoxy(&wsm.target->x, &wsm.target->y, wsm.target, arg->i);
	if (locatews(&ws, NULL, wsm.target->x, wsm.target->y, NULL)) {
		strcpy(wsm.target->name, ws->name);
	}
	updatewsmpixmap(wsm.target);
	renderwsmbox(wsm.target);
}

void
termclient(struct client *c)
{
	free(c);
}

void
termlayout(struct layout *l)
{
	XFreePixmap(kwm.dpy, l->icon_norm);
	XFreePixmap(kwm.dpy, l->icon_sel);
}

void
termmon(struct monitor *mon)
{
	free(mon);
}

void
termws(struct workspace *ws)
{
	if (ws->nc) {
		WARN("destroying non-empty workspace");
	}
	termwsmbox(ws);
	free(ws);
}

void
termwsmbox(struct workspace *ws)
{
	if (ws->wsmbox) {
		XDestroyWindow(kwm.dpy, ws->wsmbox);
		ws->wsmbox = 0;
	}
	if (ws->wsmpm) {
		XFreePixmap(kwm.dpy, ws->wsmpm);
		ws->wsmpm = 0;
	}
}

void
togglefloat(union argument const *arg)
{
	struct client *c;
	(void) arg;

	if (!selmon->selws->nc) {
		return;
	}
	c = selmon->selws->selcli;
	setfloating(c, !c->floating);
}

void
togglepad(union argument const *arg)
{
	(void) arg;

	if (!pad)
		return;
	setclientmask(selmon, false);
	if (pad_mon == selmon) {
		showclient(NULL, pad);
		pad_mon = NULL;
	} else {
		pad_mon = selmon;
		moveresizeclient(pad_mon, pad,
				pad_mon->wx + PADMARGIN, pad_mon->wy + PADMARGIN,
				pad_mon->ww - 2*PADMARGIN, pad_mon->wh - 2*PADMARGIN);
	}
	updatefocus();
	setclientmask(selmon, true);
}

void
togglewsm(union argument const *arg)
{
	unsigned int i;
	(void) arg;

	wsm.active = !wsm.active;

	if (!wsm.active) {
		for (i = 0; i < nws; i++) {
			termwsmbox(workspaces[i]);
		}
		termwsmbox(wsm.target);
		grabkeys();
		setclientmask(selmon, true);
		return;
	}

	/* create a box for each workspace */
	for (i = 0; i < nws; i++)
		initwsmbox(workspaces[i]);
	initwsmbox(wsm.target);
	XRaiseWindow(kwm.dpy, selmon->selws->wsmbox);
	XRaiseWindow(kwm.dpy, wsm.target->wsmbox);

	/* initial target is the current workspace (make a "0-step") */
	wsm.target->x = selmon->selws->x;
	wsm.target->y = selmon->selws->y;
	stepwsmbox(&((union argument const) { .i = NoDirection }));

	/* grab input */
	setclientmask(selmon, false);
	grabkeys();

	/* initial update */
	updatewsm();
}

size_t
unifyscreens(XineramaScreenInfo **list, size_t len)
{
	unsigned int i, j, n;
	bool dup;

	/* reserve enough space */
	XineramaScreenInfo *unique = calloc(len, sizeof(XineramaScreenInfo));
	if (!unique && len) {
		FATAL("could not allocate %zu bytes for screen info list",
		      len*sizeof(XineramaScreenInfo));
		exit(EXIT_FAILURE);
	}

	for (i = 0, n = 0; i < len; i++) {
		dup = false;
		for (j = 0; j < i; j++) {
			if ((*list)[j].x_org == (*list)[i].x_org
					&& (*list)[j].y_org == (*list)[i].y_org
					&& (*list)[j].width == (*list)[i].width
					&& (*list)[j].height == (*list)[i].height) {
				dup = true;
				break;
			}
		}
		if (!dup) {
			unique[n].x_org = (*list)[i].x_org;
			unique[n].y_org = (*list)[i].y_org;
			unique[n].width = (*list)[i].width;
			unique[n].height = (*list)[i].height;
			n++;
		}
	}
	XFree(*list);
	*list = unique;
	*list = realloc(*list, n*sizeof(XineramaScreenInfo)); /* fix size */
	return n;
}

void
unmapnotify(XEvent *e)
{
	DEBUG("unmapnotify(%lu)", e->xunmap.window);

	struct client *c;
	struct workspace *ws;
	struct monitor *mon;

	if (pad && e->xunmap.window == pad->win) {
		termclient(pad);
		if (pad_mon) {
			pad_mon = NULL;
			updatefocus();
		}
		pad_mon = NULL;
		pad = NULL;
		return;
	}

	if (!locateclient(&ws, &c, NULL, e->xunmap.window, NULL)) {
		return;
	}

	detachclient(c);
	if (locatemon(&mon, NULL, ws)) {
		arrange(mon);
		updatefocus();
	} else if (!c->floating) {
		ws->dirty = true;
	}
	termclient(c);
}

void
updateclient(struct client *c)
{
	int di;
	unsigned long dl;
	unsigned char *p = NULL;
	Atom da;

	if (XGetWindowProperty(kwm.dpy, c->win, netatom[NetWMState], 0, sizeof(da),
			false, XA_ATOM, &da, &di, &dl, &dl, &p) == Success && p) {
		if ((Atom) *p == netatom[NetWMStateFullscreen]) {
			setfullscreen(c, true);
		}
		free(p);
	}
	if (XGetWindowProperty(kwm.dpy, c->win, netatom[NetWMWindowType], 0, sizeof(da),
			false, XA_ATOM, &da, &di, &dl, &dl, &p) == Success && p) {
		if ((Atom) *p == netatom[NetWMWindowTypeDialog]) {
			DEBUG("\033[32mwindow %lu is a dialog!\033[0m", c->win);
			c->dialog = true;
			setfloating(c, true);
		} else {
			DEBUG("\033[32mwindow %lu is not a dialog\033[0m", c->win);
		}
	} else {
		DEBUG("\033[32mcould not get window property for %lu\033[0m", c->win);
	}
}

void
updatefocus(void)
{
	unsigned int i, j;
	bool sel;
	struct monitor *mon;

	for (i = 0; i < nmon; i++) {
		mon = monitors[i];
		sel = mon == selmon && pad_mon != selmon;

		/* empty monitor: don't focus anything */
		if (!mon->selws->ns) {
			if (sel) {
				XSetInputFocus(kwm.dpy, root, RevertToPointerRoot, CurrentTime);
			}
			continue;
		}

		/* draw borders and restack */
		for (j = 0; j < (sel ? mon->selws->ns-1 : mon->selws->ns); j++) {
			XSetWindowBorder(kwm.dpy, mon->selws->stack[j]->win, CBORDERNORM);
			grabbuttons(mon->selws->stack[j], false);
		}
		if (sel) {
			XSetWindowBorder(kwm.dpy, mon->selws->selcli->win, CBORDERSEL);
			XSetInputFocus(kwm.dpy, mon->selws->selcli->win, RevertToPointerRoot,
					CurrentTime);
			grabbuttons(mon->selws->stack[j], true);
		}
	}

	/* focus scratchpad if existent and visible */
	if (pad) {
		if (pad_mon == selmon) {
			XSetInputFocus(kwm.dpy, pad->win, RevertToPointerRoot, CurrentTime);
			XSetWindowBorder(kwm.dpy, pad->win, CBORDERSEL);
			XRaiseWindow(kwm.dpy, pad->win);
			grabbuttons(pad, true);
		} else {
			XSetWindowBorder(kwm.dpy, pad->win, CBORDERNORM);
			grabbuttons(pad, false);
		}
	}
}

void
updategeom(void)
{
	int i, n;
	XineramaScreenInfo *info;
	struct monitor *mon;

	if (!XineramaIsActive(kwm.dpy)) {
		while (nmon > 1) {
			mon = monitors[1];
			detachmon(mon);
			termmon(mon);
		}
		if (!nmon) {
			selmon = initmon();
			attachmon(selmon);
		}
		updatemon(selmon, 0, 0, DisplayWidth(kwm.dpy, screen),
				DisplayHeight(kwm.dpy, screen));
		return;
	}

	info = XineramaQueryScreens(kwm.dpy, &n);
	n = unifyscreens(&info, n);

	if (n < nmon) { /* screen detached */
		for (i = nmon-1; i >= n; i--) {
			mon = monitors[i];
			showws(NULL, mon->selws);
			detachmon(mon);
			termmon(mon);
		}
	}
	for (i = 0; i < n; i++) {
		if (i >= nmon) { /* screen attached */
			mon = initmon();
			attachmon(mon);
		}
		selmon = monitors[i];
		updatemon(selmon, info[i].x_org, info[i].y_org, info[i].width, info[i].height);
	}
	free(info);
}

void
updatemon(struct monitor *mon, int x, int y, unsigned int w, unsigned int h)
{
	mon->x = x;
	mon->y = y;
	mon->w = w;
	mon->h = h;

	mon->wx = 0;
	mon->wy = 0;
	mon->ww = mon->w;
	mon->wh = mon->h;
	arrange(mon);
}

void
updatetransient(struct client *c)
{
	Window trans=0;
	struct workspace *ws;
	struct monitor *mon;

	if (XGetTransientForHint(kwm.dpy, c->win, &trans) &&
			locateclient(&ws, NULL, NULL, trans, NULL)) {
		DEBUG("\033[33mwindow %lu is transient\033[0m", c->win);
		setfloating(c, true);
		if (locatemon(&mon, NULL, ws)) {
			detachclient(c);
			attachclient(ws, c);
		}
	} else {
		DEBUG("\033[33mwindow %lu is not transient\033[0m", c->win);
	}
	DEBUG("\033[33m=> trans = %lu\033[0m", trans);
}

void
updatewsm(void)
{
	unsigned int i;

	if (!wsm.active) {
		return;
	}

	for (i = 0; i < nws; i++) {
		updatewsmbox(workspaces[i]);
	}
	updatewsmbox(wsm.target);
	XSync(kwm.dpy, false);
}

void
updatewsmbox(struct workspace *ws)
{
	int x, y, cx, cy;

	cx = selmon->x + selmon->w/2;
	cy = selmon->y + selmon->h/2;
	x = cx + (ws->x - wsm.target->x) * (WSMBOXWIDTH + WSMBORDERWIDTH) -
			WSMBOXWIDTH/2 - WSMBORDERWIDTH;
	y = cy + (ws->y - wsm.target->y) * (WSMBOXHEIGHT + WSMBORDERWIDTH) -
			WSMBOXHEIGHT/2 - WSMBORDERWIDTH;

	XMoveWindow(kwm.dpy, ws->wsmbox, x, y);
}

void
updatewsmpixmap(struct workspace *ws)
{
	XSetForeground(kwm.dpy, dc.gc, ws == selmon->selws ? WSMCBGSEL
			: ws == wsm.target ? WSMCBGTARGET : WSMCBGNORM);
	XFillRectangle(kwm.dpy, ws->wsmpm, dc.gc, 0, 0, WSMBOXWIDTH, WSMBOXHEIGHT);
	XSetForeground(kwm.dpy, dc.gc, ws == selmon->selws ? WSMCSEL
			: ws == wsm.target ? WSMCTARGET : WSMCNORM);
	Xutf8DrawString(kwm.dpy, ws->wsmpm, dc.font.xfontset, dc.gc, 2,
			dc.font.ascent+1, ws->name, strlen(ws->name));
}

void
viewmon(struct monitor *mon)
{
	struct monitor *oldmon = selmon;
	selmon = mon;
}

void
viewws(union argument const *arg)
{
	(void) arg;

	if (!wsm.active)
		return;
	setws(selmon, wsm.target->x, wsm.target->y);
	togglewsm(NULL);
}

int
xerror(Display *dpy, XErrorEvent *ee)
{
	char es[256];

	/* catch certain errors */
	if (ee->error_code == BadWindow
	|| (ee->request_code == X_SetInputFocus && ee->error_code == BadMatch)
	|| (ee->request_code == X_PolyText8 && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolyFillRectangle && ee->error_code == BadDrawable)
	|| (ee->request_code == X_PolySegment && ee->error_code == BadDrawable)
	|| (ee->request_code == X_ConfigureWindow && ee->error_code == BadMatch)
	|| (ee->request_code == X_GrabButton && ee->error_code == BadAccess)
	|| (ee->request_code == X_GrabKey && ee->error_code == BadAccess)
	|| (ee->request_code == X_CopyArea && ee->error_code == BadDrawable)) {
		XGetErrorText(dpy, ee->error_code, es, 256);
		WARN("%s (ID %d) after request %d", es, ee->error_code, ee->error_code);
		return 0;
	}
	return xerrorxlib(dpy, ee); /* default error handler (might call exit) */
}

void
zoom(union argument const *arg)
{
	unsigned int pos;
	struct client *c;
	(void) arg;

	if (selmon->selws->nc == 0) {
		return;
	}

	if (!locateclient(NULL, &c, &pos, selmon->selws->selcli->win, NULL)) {
		WARN("attempt to zoom non-existing window %d",
		     selmon->selws->selcli->win);
		return;
	}

	if (pos == 0) {
		/* window is at the top: swap with next below */
		if (selmon->selws->nc > 1) {
			selmon->selws->clients[0] = selmon->selws->clients[1];
			selmon->selws->clients[1] = c;
			push(selmon->selws, selmon->selws->clients[0]);
			updatefocus();
		} else {
			return;
		}
	} else {
		/* window is somewhere else: swap with top */
		selmon->selws->clients[pos] = selmon->selws->clients[0];
		selmon->selws->clients[0] = c;
	}
	arrange(selmon);
}

int
main(int argc, char **argv)
{
	char *sessionfile=NULL;
	log_level = LOG_DEBUG;

	if (argc == 2) {
		if (argv[1][0] == '-') {
			puts(APPNAME" © 2014 ayekat, see LICENSE for details");
			return EXIT_FAILURE;
		}
		sessionfile = argv[1];
	}
	if (!(kwm.dpy = XOpenDisplay(NULL))) {
		FATAL("could not open X");
		return EXIT_FAILURE;
	}
	NOTE("starting %s...", APPNAME);
	setup(sessionfile);
	run();
	cleanup(&sessionfile);
	XCloseDisplay(kwm.dpy);
	if (restarting) {
		NOTE("restarting %s...", APPNAME);
		execlp(APPNAME, APPNAME, sessionfile, NULL);
	}
	NOTE("shutting down %s...", APPNAME);
	return EXIT_SUCCESS;
}
