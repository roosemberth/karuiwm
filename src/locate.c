#include "locate.h"

bool
locate_window2client(struct client **clients, size_t nc, Window win,
                     struct client **client, int unsigned *index)
{
	int unsigned i;

	for (i = 0; i < nc; ++i) {
		if (clients[i]->win == win) {
			if (index != NULL)  *index = i;
			if (client != NULL) *client = clients[i];
			return true;
		}
	}
	return false;
}

/* TODO workspaces, pages */
bool
locate_client2index(struct client **clients, size_t nc, struct client *c,
                    int unsigned *index)
{
	int unsigned i;

	for (i = 0; i < nc && clients[i] != c; ++i);
	if (i == nc)
		return false;
	*index = i;
	return true;
}
