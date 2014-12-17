#ifndef _LOCATE_H
#define _LOCATE_H

#include "karuiwm.h"

bool locate_window2client(struct client **clients, size_t nc, Window win,
                          struct client **client, int unsigned *index);
bool locate_client2index(struct client **clients, size_t nc, struct client *c,
                         int unsigned *index);

#endif /* _LOCATE_H */
