/* stwm configuration */

static char const *font = "-misc-fixed-medium-r-semicondensed--13-100-100-100-c-60-iso8859-1";
static unsigned int nmaster = 1; /* number of windows in the master area */
static float mfact = 0.5;        /* size of master area */
static char const *wsnames[] = { "alpha", "beta", "gamma", "delta", "epsilon",
                                 "zeta", "eta", "theta", "iota", "kappa",
                                 "lambda", "mu", "nu", "xi", "omicron", "pi",
                                 "rho", "sigma", "tau", "upsilon", "phi", "chi",
                                 "psi", "omega" };

/* colours */
static unsigned int borderwidth  = 1;
static unsigned long cbordernorm = 0x222222;
static unsigned long cbordersel  = 0xAFD700;

/* commands */
static char const *termcmd[] = { "xterm", NULL };
static char const *dmenucmd[] = { "dmenu_run", NULL };

#define MODKEY Mod1Mask
static Key keys[] = {
	/* applications */
	{ MODKEY,             XK_n,      spawn,      { .v=termcmd } },
	{ MODKEY,             XK_p,      spawn,      { .v=dmenucmd } },

	/* windows */
	{ MODKEY,             XK_j,      focusstep,  { .i=+1 } },
	{ MODKEY,             XK_k,      focusstep,  { .i=-1 } },
	{ MODKEY,             XK_l,      setmfact,   { .f=+0.02 } },
	{ MODKEY,             XK_h,      setmfact,   { .f=-0.02 } },
	{ MODKEY|ShiftMask,   XK_i,      killclient, { 0 } },

	/* layout */
	{ MODKEY|ShiftMask,   XK_j,      shift,      { .i=+1 } },
	{ MODKEY|ShiftMask,   XK_k,      shift,      { .i=-1 } },
	{ MODKEY,             XK_comma,  setnmaster, { .i=+1 } },
	{ MODKEY,             XK_period, setnmaster, { .i=-1 } },
	{ MODKEY,             XK_Return, zoom,       { 0 } },

	/* workspaces */
	{ MODKEY,             XK_space,  wsd_toggle, { 0 } },
	{ MODKEY|ControlMask, XK_h,      ws_step,    { .i=WSSTEP_LEFT } },
	{ MODKEY|ControlMask, XK_l,      ws_step,    { .i=WSSTEP_RIGHT } },
	{ MODKEY|ControlMask, XK_j,      ws_step,    { .i=WSSTEP_DOWN } },
	{ MODKEY|ControlMask, XK_k,      ws_step,    { .i=WSSTEP_UP } },

	/* session */
	{ MODKEY,             XK_q,      restart,    { 0 } },
	{ MODKEY|ShiftMask,   XK_q,      quit,       { 0 } },
};

/* workspace dialog */
static int wsdradius = 5;     /* workspaces around the centre */

