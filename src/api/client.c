#include "client.h"
#include "../util.h"

/* TODO define this elsewhere, make context-specific buffer sizes */
#define BUFSIZE 1024

void
api_client_delete(struct api_client *c)
{
	sfree(c);
}

struct api_client *
api_client_new(int socket)
{
	struct api_client *c;

	c = smalloc(sizeof(struct api_client), "API client");
	c->socket = socket;
	c->subscriptions = 0x00000000;
	return c;
}

struct api_message *
api_client_read(struct api_client *c)
{
	char msgbuf[BUFSIZE];
	int msglen;

	msglen = ipc_read(c->socket, BUFSIZE, msgbuf);
	if (msglen <= 0)
		return NULL;
	return api_message_deserialise(msgbuf);
}

int
api_client_send(struct api_client *c, struct api_message *msg)
{
	char msgbuf[BUFSIZE];
	size_t msglen;

	msglen = api_message_serialise(msg, BUFSIZE, msgbuf);
	if (msglen == 0) {
		WARN("cannot serialise message");
		return -1;
	}

	return ipc_send(c->socket, msglen, msgbuf);
}
