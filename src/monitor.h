#ifndef _MONITOR_H
#define _MONITOR_H

#include "karuiwm.h"
#include "workspace.h"

struct monitor {
	struct workspace *selws;
	int x, y, wx, wy;
	int unsigned w, h, ww, wh;
	/* prefix with w: workspace dimension */
};

#endif /* _MONITOR_H */
