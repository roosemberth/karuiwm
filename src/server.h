#ifndef _SERVER_H
#define _SERVER_H

#include "api/client.h"
#include "focus.h"
#include "session.h"

struct server {
	size_t nc;
	struct api_client *clients;
	int socket;
	struct session *session;
	struct focus *focus;
};

void server_attach_client(struct server *s, struct api_client *c);
void server_delete(struct server *s);
void server_detach_client(struct server *s, struct api_client *c);
int server_handle_connection(struct server *s);
void server_handle_message(struct server *s, int socket);
struct server *server_new(struct session *session, struct focus *focus,
                          char const *path);

#endif /* _API_SERVER_H */
