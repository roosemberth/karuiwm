#ifndef _SESSION_H
#define _SESSION_H

#include "workspace.h"
#include "client.h"
#include <X11/Xlib.h>

struct session {
	size_t nws;
	struct workspace *workspaces;
};

void session_attach_workspace(struct session *s, struct workspace *ws);
void session_delete(struct session *s);
void session_detach_workspace(struct session *s, struct workspace *ws);
int session_locate_window(struct session *s, struct client **c, Window w);
struct session *session_new(void);
int session_save(struct session *s, char *sid, size_t sid_len);

#endif /* ndef _SESSION_H */
