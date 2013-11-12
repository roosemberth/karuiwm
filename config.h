#define MODKEY Mod4Mask  /* main modifier key for stwm */
int nmaster = 1;         /* number of windows in the master area */
float mfact = 0.6;       /* size of master area */

Key keys[] = {
	{ MODKEY,           XK_j,       focusstep,   { .i=+1 } },
	{ MODKEY,           XK_k,       focusstep,   { .i=-1 } },
	{ MODKEY,           XK_l,       setmfact,    { .f=+0.02 } },
	{ MODKEY,           XK_h,       setmfact,    { .f=-0.02 } },
	{ MODKEY,           XK_r,       restart,     { 0 } },
	{ MODKEY,           XK_q,       quit,        { 0 } },
};

