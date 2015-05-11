#ifndef _API_MESSAGE_H
#define _API_MESSAGE_H

#include "api.h"

struct api_message {
	enum api_command command;
	enum api_property property;
	size_t nint, nstr;
	int *argv_int;
	char **argv_str;
};

void api_message_delete(struct api_message *msg);
struct api_message *api_message_deserialise(char const *data);
struct api_message *api_message_new(enum api_command cmd,
                                    enum api_property prop, ...);
size_t api_message_serialise(struct api_message *msg, size_t buflen, char *buf);

#endif /* _API_MESSAGE_H */
