#ifndef _API_CLIENT_H
#define _API_CLIENT_H

#include "ipc.h"
#include "message.h"
#include <stdint.h>

struct api_client {
	struct api_client *prev, *next;
	int socket;
	uint32_t subscriptions;
};

void api_client_delete(struct api_client *c);
struct api_message *api_client_read(struct api_client *c);
struct api_client *api_client_new(int socket);
int api_client_send(struct api_client *c, struct api_message *msg);

#endif /* _API_CLIENT_H */
