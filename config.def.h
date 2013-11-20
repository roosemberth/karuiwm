/* stwm configuration */

static char const *font = "-misc-fixed-medium-r-semicondensed--13-100-100-100-c-60-iso8859-1";
static unsigned int const nmaster = 1;      /* #windows in the master area */
static float const mfact = 0.5;             /* size of master area */
static unsigned int const borderwidth  = 1; /* window border width */
static char const *wsnames[] = { "alpha", "beta", "gamma", "delta", "epsilon",
                                 "zeta", "eta", "theta", "iota", "kappa",
                                 "lambda", "mu", "nu", "xi", "omicron", "pi",
                                 "rho", "sigma", "tau", "upsilon", "phi", "chi",
                                 "psi", "omega" };

/* colours */
#define cbordernorm 0x222222
#define cbordersel  0xAFD700

/* commands */
static char const *termcmd[] = { "xterm", NULL };
static char const *dmenucmd[] = { "dmenu_run", NULL };

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
	{ MODKEY,             XK_n,      spawn,      { .v=termcmd } },
	{ MODKEY,             XK_p,      spawn,      { .v=dmenucmd } },

	/* windows */
	{ MODKEY,             XK_j,      stepfocus,  { .i=+1 } },
	{ MODKEY,             XK_k,      stepfocus,  { .i=-1 } },
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
	{ MODKEY,             XK_space,  togglewsd,  { 0 } },
	{ MODKEY|ControlMask, XK_h,      stepws,     { .i=LEFT } },
	{ MODKEY|ControlMask, XK_l,      stepws,     { .i=RIGHT } },
	{ MODKEY|ControlMask, XK_j,      stepws,     { .i=DOWN } },
	{ MODKEY|ControlMask, XK_k,      stepws,     { .i=UP } },

	/* session */
	{ MODKEY,             XK_q,      restart,    { 0 } },
	{ MODKEY|ShiftMask,   XK_q,      quit,       { 0 } },
};

/* workspace dialog keys */
static Key const wsdkeys[] = {
	{ 0,                  XK_Escape, togglewsd,  { 0 } },
	{ MODKEY,             XK_space,  togglewsd,  { 0 } },

	/* moving view */
	{ MODKEY,             XK_h,      selectws,   { .i=LEFT } },
	{ MODKEY,             XK_l,      selectws,   { .i=RIGHT } },
	{ MODKEY,             XK_j,      selectws,   { .i=DOWN } },
	{ MODKEY,             XK_k,      selectws,   { .i=UP } },
};

/* workspace dialog */
static int const wsdradius = 5;     /* workspaces around the centre */
static int const wsdborder = 1;     /* border width of one box */

/* workspace dialog - colours */
#define wsdcbordernorm   0x555555
#define wsdcbgnorm       cbordernorm
#define wsdcnorm         0x888888

#define wsdcbordersel    0xAAAAAA
#define wsdcbgsel        0x333333
#define wsdcsel          0xAAAAAA

#define wsdcbordertarget cbordersel
#define wsdcbgtarget     0x334000
#define wsdctarget       cbordersel

