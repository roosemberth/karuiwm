/* stwm configuration */

static unsigned int nmaster = 1; /* number of windows in the master area */
static float mfact = 0.6;        /* size of master area */

/* colours */
static unsigned int borderwidth  = 1;
static unsigned long cbordernorm = 0x222222;
static unsigned long cbordersel  = 0xAFD700;

/* commands */
static char const *termcmd[] = { "xterm", NULL };
static char const *dmenucmd[] = { "dmenu_run", NULL };

#define MODKEY Mod4Mask
static Key keys[] = {
	/* applications */
	{ MODKEY,           XK_n,       spawn,       { .v=termcmd } },
	{ MODKEY,           XK_p,       spawn,       { .v=dmenucmd } },

	/* windows */
	{ MODKEY,           XK_j,       focusstep,   { .i=+1 } },
	{ MODKEY,           XK_k,       focusstep,   { .i=-1 } },
	{ MODKEY,           XK_l,       setmfact,    { .f=+0.02 } },
	{ MODKEY,           XK_h,       setmfact,    { .f=-0.02 } },
	{ MODKEY,           XK_Return,  zoom,        { 0 } },

	/* session */
	{ MODKEY,           XK_q,       restart,     { 0 } },
	{ MODKEY|ShiftMask, XK_q,       quit,        { 0 } },
};

