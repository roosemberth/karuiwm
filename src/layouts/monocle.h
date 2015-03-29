#ifndef _LAYOUT_MONOCLE_H
#define _LAYOUT_MONOCLE_H

#include "../karuiwm.h"
#include "../client.h"

void monocle(struct client *clients, size_t nc, size_t nmaster, float mfact,
             int sx, int sy, int unsigned sw, int unsigned sh);

#endif /* ndef _LAYOUT_MONOCLE_H */
