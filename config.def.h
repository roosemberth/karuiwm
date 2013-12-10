/* stwm configuration */

#define FONTSTR "-misc-fixed-medium-r-semicondensed--13-100-100-100-c-60-iso8859-1"
#define NMASTER 1        /* number of clients in master area */
#define MFACT 0.5        /* size of master area */
#define FORCESIZE true   /* force terminals to fit the layout? */
#define BORDERWIDTH 1    /* window border width */
#define WSMBORDERWIDTH 1 /* WSM box border width */
#define WSMRADIUS 5      /* number of workspaces around WSM centre */

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

/* layouts */
static void (*layouts[])(Monitor *) = {
	rstack, /* stack at right [default] */
	bstack, /* stack at bottom */
	NULL,   /* steplayout() will not cycle beyond this point */
};

/* default workspace name */
#define DEFAULT_WSNAME "[no name]"

/* dmenu arguments (see man dmenu) */
#define PROMPT_RENAME "rename"
#define PROMPT_CHANGE "workspace"
#define PROMPT_SPAWN  "spawn"
static char const *dmenuargs[] = { "-b", "-l", "10",
	                               "-nf", "#888888", "-nb", "#222222",
	                               "-sf", "#AFD800", "-sb", "#444444", NULL };

/* commands */
static char const *termcmd[] = { "xterm", NULL };
static char const *scrotcmd[] = { "scrot", NULL };

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
	{ MODKEY,                       XK_n,      spawn,       { .v=termcmd } },
	{ MODKEY,                       XK_p,      dmenu,       { .i=DMENU_SPAWN } },
	{ 0,                            XK_Print,  spawn,       { .v=scrotcmd } },

	/* windows */
	{ MODKEY,                       XK_j,      stepfocus,   { .i=+1 } },
	{ MODKEY,                       XK_k,      stepfocus,   { .i=-1 } },
	{ MODKEY,                       XK_l,      setmfact,    { .f=+0.02 } },
	{ MODKEY,                       XK_h,      setmfact,    { .f=-0.02 } },
	{ MODKEY,                       XK_t,      togglefloat, { 0 } },
	{ MODKEY|ShiftMask,             XK_c,      killclient,  { 0 } },

	/* layout */
	{ MODKEY|ShiftMask,             XK_j,      shift,       { .i=+1 } },
	{ MODKEY|ShiftMask,             XK_k,      shift,       { .i=-1 } },
	{ MODKEY,                       XK_comma,  setnmaster,  { .i=+1 } },
	{ MODKEY,                       XK_period, setnmaster,  { .i=-1 } },
	{ MODKEY,                       XK_z,      zoom,        { 0 } },
	{ MODKEY,                       XK_space,  steplayout,  { .i=+1 } },
	{ MODKEY|ShiftMask,             XK_space,  steplayout,  { .i=-1 } },

	/* workspaces */
	{ MODKEY,                       XK_o,      togglewsm,   { 0 } },
	{ MODKEY|ControlMask,           XK_h,      stepws,      { .i=LEFT } },
	{ MODKEY|ControlMask,           XK_l,      stepws,      { .i=RIGHT } },
	{ MODKEY|ControlMask,           XK_j,      stepws,      { .i=DOWN } },
	{ MODKEY|ControlMask,           XK_k,      stepws,      { .i=UP } },
	{ MODKEY|ControlMask|ShiftMask, XK_h,      moveclient,  { .i=LEFT } },
	{ MODKEY|ControlMask|ShiftMask, XK_l,      moveclient,  { .i=RIGHT } },
	{ MODKEY|ControlMask|ShiftMask, XK_j,      moveclient,  { .i=DOWN } },
	{ MODKEY|ControlMask|ShiftMask, XK_k,      moveclient,  { .i=UP } },
	{ MODKEY,                       XK_Return, dmenu,       { .i=DMENU_VIEW } },
	{ MODKEY|ShiftMask,             XK_Return, dmenu,       { .i=DMENU_RENAME } },

	/* monitors */
	{ MODKEY,                       XK_m,      stepmon,     { 0 } },

	/* session */
	{ MODKEY,                       XK_q,      restart,     { 0 } },
	{ MODKEY|ShiftMask,             XK_q,      quit,        { 0 } },
};

/* WSM keys */
static Key const wsmkeys[] = {
	{ 0,                            XK_Escape, togglewsm,   { 0 } },
	{ MODKEY,                       XK_o,      togglewsm,   { 0 } },
	{ 0,                            XK_Return, viewws,      { 0 } },

	/* move view */
	{ MODKEY,                       XK_h,      stepwsmbox,  { .i=LEFT } },
	{ MODKEY,                       XK_l,      stepwsmbox,  { .i=RIGHT } },
	{ MODKEY,                       XK_j,      stepwsmbox,  { .i=DOWN } },
	{ MODKEY,                       XK_k,      stepwsmbox,  { .i=UP } },

	/* move workspace */
	{ MODKEY|ShiftMask,             XK_h,      movews,      { .i=LEFT } },
	{ MODKEY|ShiftMask,             XK_l,      movews,      { .i=RIGHT } },
	{ MODKEY|ShiftMask,             XK_j,      movews,      { .i=DOWN } },
	{ MODKEY|ShiftMask,             XK_k,      movews,      { .i=UP } },
};

/* mouse buttons */
static Button const buttons[] = {
	{ MODKEY,                       Button1,   movemouse,   { 0 } },
	{ MODKEY,                       Button3,   resizemouse, { 0 } },
};

