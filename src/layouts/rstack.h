#ifndef _LAYOUT_RSTACK_H
#define _LAYOUT_RSTACK_H

#include "../karuiwm.h"

void rstack(struct client **clients, size_t nc, size_t nmaster, float mfact,
            int unsigned sw, int unsigned sh);

#endif /* _LAYOUT_RSTACK_H */
