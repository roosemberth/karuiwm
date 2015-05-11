#include "server.h"
#include "api/ipc.h"
#include "karuiwm.h"
#include "util.h"
#include "list.h"

static int handle_message(struct server *s, struct api_client *c,
                          struct api_message *msg);
static int handle_message_get(struct server *s, struct api_client *c,
                              struct api_message *msg);
static int handle_message_set(struct server *s, struct api_client *c,
                              struct api_message *msg);

static int
handle_message(struct server *s, struct api_client *c, struct api_message *msg)
{
	switch (msg->command) {
	case API_COMMAND_GET:
		handle_message_get(s, c, msg);
		break;
	case API_COMMAND_SET:
		handle_message_set(s, c, msg);
		break;
	default:
		return -1;
	}
	return 0;
}

static int
handle_message_get(struct server *s, struct api_client *c,
                   struct api_message *msg)
{
	struct api_message *reply;

	switch (msg->property) {
	case API_PROPERTY_MONITOR:
		reply = api_message_new(API_COMMAND_SET, API_PROPERTY_MONITOR,
		                        s->focus->selmon->index);
		api_client_send(c, reply);
		break;
	default:
		WARN("%X,%X: not implemented", msg->command, msg->property);
	}
	return 0;
}

static int
handle_message_set(struct server *s, struct api_client *c,
                   struct api_message *msg)
{
	(void) s;
	(void) c;

	WARN("%X,%X: not implemented", msg->command, msg->property);
	return 0;
}


void
server_attach_client(struct server *s, struct api_client *c)
{
	LIST_APPEND(s->clients, c);
	++s->nc;
}

void
server_delete(struct server *s)
{
	struct api_client *c;

	while (s->nc > 0) {
		c = s->clients;
		server_detach_client(s, c);
		api_client_delete(c);
	}
}

void
server_detach_client(struct server *s, struct api_client *c)
{
	LIST_REMOVE(s->clients, c);
	--s->nc;
}

int
server_handle_connection(struct server *s)
{
	int sock;
	struct api_client *c;

	sock = ipc_accept(s->socket);
	if (sock < 0) {
		WARN("cannot accept new connection because of underlying error");
		return 0;
	}

	c = api_client_new(sock);
	server_attach_client(s, c);
	return sock;
}

void
server_handle_message(struct server *s, int socket)
{
	struct api_client *c;
	struct api_message *msg;
	int i;

	for (c = s->clients; c != NULL && c->socket != socket; c = c->next);
	if (c == NULL) {
		WARN("attempting to treat unhandled socket %d", socket);
		return;
	}
	msg = api_client_read(c);
	if (msg == NULL) {
		DEBUG("closing API client %d", socket);
		server_detach_client(s, c);
		api_client_delete(c);
		return;
	}
	i = handle_message(s, c, msg);
	if (i < 0)
		WARN("received malformed message from API client %d", socket);
}

struct server *
server_new(struct session *session, struct focus *focus, char const *path)
{
	int csock;
	struct server *s;

	csock = ipc_init(path);
	if (csock < 0) {
		ERROR("cannot create API server because of underlying error");
		return NULL;
	}

	s = smalloc(sizeof(struct server), "API server");
	s->nc = 0;
	s->clients = NULL;
	s->socket = csock;
	s->session = session;
	s->focus = focus;
	return s;
}
