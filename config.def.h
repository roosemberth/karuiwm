/* stwm configuration */

#define FONTSTR "-misc-fixed-medium-r-semicondensed--13-100-100-100-c-60-iso8859-1"
#define NMASTER 1        /* number of clients in master area */
#define MFACT 0.5        /* size of master area */
#define FORCESIZE true   /* force terminals to fit the layout? */
#define BORDERWIDTH 1    /* window border width */
#define WSMBORDERWIDTH 1 /* WSM box border width */
#define WSMRADIUS 4      /* number of workspaces around WSM centre */
#define PADMARGIN 20     /* border gap for scratchpad workspace */

/* colours */
#define CBORDERNORM      0x222222   /* normal windows */
#define CBORDERSEL       0xAFD700   /* selected windows */

#define CNORM            0x888888   /* status bar */
#define CBGNORM          0x222222

#define CSEL             0xCCCCCC
#define CBGSEL           0x444444

#define WSMCNORM         CNORM      /* WSM box of normal workspaces */
#define WSMCBGNORM       CBGNORM
#define WSMCBORDERNORM   CBGSEL

#define WSMCSEL          CSEL       /* WSM box of current workspace */
#define WSMCBGSEL        CBGSEL
#define WSMCBORDERSEL    CSEL

#define WSMCTARGET       CBORDERSEL /* WSM box of selected workspace */
#define WSMCBGTARGET     CBGSEL
#define WSMCBORDERTARGET CBORDERSEL

/* icon for rstack layout */
static long const icon_rstack[] = {
	17, 15,
	0x00000,
	0x00000,
	0x1FEFF,
	0x1FEFF,
	0x1FEFF,
	0x1FE00,
	0x1FEFF,
	0x1FEFF,
	0x1FEFF,
	0x1FE00,
	0x1FEFF,
	0x1FEFF,
	0x1FEFF,
	0x00000,
	0x00000,
};

/* icon for bstack layout */
static long const icon_bstack[] = {
	17, 15,
	0x00000,
	0x00000,
	0x1FFFF,
	0x1FFFF,
	0x1FFFF,
	0x1FFFF,
	0x1FFFF,
	0x00000,
	0x1F7DF,
	0x1F7DF,
	0x1F7DF,
	0x1F7DF,
	0x1F7DF,
	0x00000,
	0x00000,
};

/* layouts */
static Layout layouts[] = {
	{ icon_rstack, rstack },
	{ icon_bstack, bstack },
	{ NULL,        NULL },
};

/* dmenu arguments (see man dmenu) */
#define PROMPT_RENAME "rename"
#define PROMPT_CHANGE "workspace"
#define PROMPT_SPAWN  "spawn"
static char const *dmenuargs[] = { "-l", "10",
	                               "-nf", "#888888", "-nb", "#222222",
	                               "-sf", "#AFD800", "-sb", "#444444", NULL };

/* commands */
static char const *termcmd[] = { "xterm", NULL };
static char const *scrotcmd[] = { "scrot", NULL };
static char const *volupcmd[] = { "amixer", "set", "Master", "2+", "unmute", NULL };
static char const *voldowncmd[] = { "amixer", "set", "Master", "2-", "unmute", NULL };
static char const *volmutecmd[] = { "amixer", "set", "Master", "toggle", NULL };

/* custom behaviour */
static void
custom_startup()
{
	/* place code here */
}

static void
custom_shutdown()
{
	/* place code here */
}

#define MODKEY Mod1Mask

/* normal keys */
static Key const keys[] = {
	/* applications */
	{ MODKEY,                       XK_n,       spawn,            { .v=termcmd } },
	{ MODKEY,                       XK_p,       dmenu,            { .i=DMENU_SPAWN } },
	{ 0,                            XK_Print,   spawn,            { .v=scrotcmd } },

	/* hardware */
	{ 0,                            0x1008FF11, spawn,            { .v=voldowncmd } },
	{ 0,                            0x1008FF12, spawn,            { .v=volmutecmd } },
	{ 0,                            0x1008FF13, spawn,            { .v=volupcmd } },

	/* windows */
	{ MODKEY,                       XK_j,       stepfocus,        { .i=+1 } },
	{ MODKEY,                       XK_k,       stepfocus,        { .i=-1 } },
	{ MODKEY,                       XK_l,       setmfact,         { .f=+0.02 } },
	{ MODKEY,                       XK_h,       setmfact,         { .f=-0.02 } },
	{ MODKEY,                       XK_t,       togglefloat,      { 0 } },
	{ MODKEY|ShiftMask,             XK_c,       killclient,       { 0 } },

	/* layout */
	{ MODKEY|ShiftMask,             XK_j,       shiftclient,      { .i=+1 } },
	{ MODKEY|ShiftMask,             XK_k,       shiftclient,      { .i=-1 } },
	{ MODKEY,                       XK_comma,   setnmaster,       { .i=+1 } },
	{ MODKEY,                       XK_period,  setnmaster,       { .i=-1 } },
	{ MODKEY,                       XK_Return,  zoom,             { 0 } },
	{ MODKEY,                       XK_space,   steplayout,       { .i=+1 } },
	{ MODKEY|ShiftMask,             XK_space,   steplayout,       { .i=-1 } },

	/* workspaces */
	{ MODKEY|ControlMask,           XK_h,       stepws,           { .i=LEFT } },
	{ MODKEY|ControlMask,           XK_l,       stepws,           { .i=RIGHT } },
	{ MODKEY|ControlMask,           XK_j,       stepws,           { .i=DOWN } },
	{ MODKEY|ControlMask,           XK_k,       stepws,           { .i=UP } },
	{ MODKEY|ControlMask|ShiftMask, XK_h,       sendfollowclient, { .i=LEFT } },
	{ MODKEY|ControlMask|ShiftMask, XK_l,       sendfollowclient, { .i=RIGHT } },
	{ MODKEY|ControlMask|ShiftMask, XK_j,       sendfollowclient, { .i=DOWN } },
	{ MODKEY|ControlMask|ShiftMask, XK_k,       sendfollowclient, { .i=UP } },
	{ MODKEY,                       XK_o,       togglewsm,        { 0 } },
	{ MODKEY,                       XK_i,       dmenu,            { .i=DMENU_VIEW } },
	{ MODKEY|ShiftMask,             XK_i,       dmenu,            { .i=DMENU_RENAME } },

	/* scratchpad */
	{ MODKEY,                       XK_Tab,     togglepad,        { 0 } },
	{ MODKEY|ShiftMask,             XK_Tab,     setpad,           { 0 } },

	/* monitors */
	{ MODKEY,                       XK_m,       stepmon,          { 0 } },

	/* session */
	{ MODKEY,                       XK_q,       restart,          { 0 } },
	{ MODKEY|ShiftMask,             XK_q,       quit,             { 0 } },
};

/* WSM keys */
static Key const wsmkeys[] = {
	{ 0,                            XK_Print,   spawn,            { .v=scrotcmd } },

	{ 0,                            XK_Escape,  togglewsm,        { 0 } },
	{ 0,                            XK_Return,  viewws,           { 0 } },

	/* move view */
	{ MODKEY,                       XK_h,       stepwsmbox,       { .i=LEFT } },
	{ MODKEY,                       XK_l,       stepwsmbox,       { .i=RIGHT } },
	{ MODKEY,                       XK_j,       stepwsmbox,       { .i=DOWN } },
	{ MODKEY,                       XK_k,       stepwsmbox,       { .i=UP } },

	/* move workspace */
	{ MODKEY|ShiftMask,             XK_h,       shiftws,          { .i=LEFT } },
	{ MODKEY|ShiftMask,             XK_l,       shiftws,          { .i=RIGHT } },
	{ MODKEY|ShiftMask,             XK_j,       shiftws,          { .i=DOWN } },
	{ MODKEY|ShiftMask,             XK_k,       shiftws,          { .i=UP } },
};

/* mouse buttons */
static Button const buttons[] = {
	{ MODKEY,                       Button1,    movemouse,        { 0 } },
	{ MODKEY,                       Button3,    resizemouse,      { 0 } },
};

