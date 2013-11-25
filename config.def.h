/* stwm configuration */

/* layout */
#define FONTSTR "-misc-fixed-medium-r-semicondensed--13-100-100-100-c-60-iso8859-1"
#define NMASTER 1        /* number of clients in master area */
#define MFACT 0.5        /* size of master area */
#define FORCESIZE true   /* force terminals to fit the layout? */
#define BORDERWIDTH 1    /* window border width */
#define WSDBORDERWIDTH 1 /* WSD box border width */
#define WSDRADIUS 5      /* number of workspaces around WSD centre */

/* colours */
#define CBORDERNORM      0x222222   /* normal windows */
#define CBORDERSEL       0xAFD700   /* selected windows */

#define CNORM            0x888888   /* status bar */
#define CBGNORM          0x222222

#define CSEL             0xCCCCCC   /* input bar (e.g. WSD bar) */
#define CBGSEL           0x444444

#define WSDCNORM         CNORM      /* WSD box of normal workspaces */
#define WSDCBGNORM       CBGNORM
#define WSDCBORDERNORM   CBGSEL

#define WSDCSEL          CSEL       /* WSD box of current workspace */
#define WSDCBGSEL        CBGSEL
#define WSDCBORDERSEL    CSEL

#define WSDCTARGET       CBORDERSEL /* WSD box of selected workspace */
#define WSDCBGTARGET     CBGSEL
#define WSDCBORDERTARGET CBORDERSEL

/* commands */
static char const *termcmd[] = { "xfce4-terminal", NULL };
static char const *dmenucmd[] = { "dmenu_run", NULL };
static char const *scrotcmd[] = { "scrot", NULL };

/* default workspace names */
static char const *wsnames[] = { "alpha", "beta", "gamma", "delta", "epsilon",
                                 "zeta", "eta", "theta", "iota", "kappa",
                                 "lambda", "mu", "nu", "xi", "omicron", "pi",
                                 "rho", "sigma", "tau", "upsilon", "phi", "chi",
                                 "psi", "omega" };

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
	{ MODKEY,                       XK_n,      spawn,      { .v=termcmd } },
	{ MODKEY,                       XK_p,      spawn,      { .v=dmenucmd } },
	{ 0,                            XK_Print,  spawn,      { .v=scrotcmd } },

	/* windows */
	{ MODKEY,                       XK_j,      stepfocus,  { .i=+1 } },
	{ MODKEY,                       XK_k,      stepfocus,  { .i=-1 } },
	{ MODKEY,                       XK_l,      setmfact,   { .f=+0.02 } },
	{ MODKEY,                       XK_h,      setmfact,   { .f=-0.02 } },
	{ MODKEY|ShiftMask,             XK_c,      killclient, { 0 } },

	/* layout */
	{ MODKEY|ShiftMask,             XK_j,      shift,      { .i=+1 } },
	{ MODKEY|ShiftMask,             XK_k,      shift,      { .i=-1 } },
	{ MODKEY,                       XK_comma,  setnmaster, { .i=+1 } },
	{ MODKEY,                       XK_period, setnmaster, { .i=-1 } },
	{ MODKEY,                       XK_Return, zoom,       { 0 } },

	/* workspaces */
	{ MODKEY,                       XK_space,  togglewsd,  { 0 } },
	{ MODKEY|ControlMask,           XK_h,      stepws,     { .i=LEFT } },
	{ MODKEY|ControlMask,           XK_l,      stepws,     { .i=RIGHT } },
	{ MODKEY|ControlMask,           XK_j,      stepws,     { .i=DOWN } },
	{ MODKEY|ControlMask,           XK_k,      stepws,     { .i=UP } },
	{ MODKEY|ControlMask|ShiftMask, XK_h,      move,       { .i=LEFT } },
	{ MODKEY|ControlMask|ShiftMask, XK_l,      move,       { .i=RIGHT } },
	{ MODKEY|ControlMask|ShiftMask, XK_j,      move,       { .i=DOWN } },
	{ MODKEY|ControlMask|ShiftMask, XK_k,      move,       { .i=UP } },

	/* session */
	{ MODKEY,                       XK_q,      restart,    { 0 } },
	{ MODKEY|ShiftMask,             XK_q,      quit,       { 0 } },
};

/* WSD keys */
static Key const wsdkeys[] = {
	{ 0,                            XK_Print,  spawn,      { .v=scrotcmd } },
	{ 0,                            XK_Escape, togglewsd,  { 0 } },
	{ MODKEY,                       XK_space,  togglewsd,  { 0 } },

	/* move view */
	{ MODKEY,                       XK_h,      stepwsdbox, { .i=LEFT } },
	{ MODKEY,                       XK_l,      stepwsdbox, { .i=RIGHT } },
	{ MODKEY,                       XK_j,      stepwsdbox, { .i=DOWN } },
	{ MODKEY,                       XK_k,      stepwsdbox, { .i=UP } },

	/* move workspace */
	{ MODKEY|ShiftMask,             XK_h,      movews,     { .i=LEFT } },
	{ MODKEY|ShiftMask,             XK_l,      movews,     { .i=RIGHT } },
	{ MODKEY|ShiftMask,             XK_j,      movews,     { .i=DOWN } },
	{ MODKEY|ShiftMask,             XK_k,      movews,     { .i=UP } },
};

