#include "message.h"
#include "api.h"
#include "../util.h"
#include <stdarg.h>
#include <string.h>

void
api_message_delete(struct api_message *msg)
{
	int unsigned i;

	if (msg->nint > 0)
		sfree(msg->argv_int);
	if (msg->nstr > 0) {
		for (i = 0; i < msg->nstr; ++i)
			sfree(msg->argv_str[i]);
		sfree(msg->argv_str);
	}
	sfree(msg);
}

struct api_message *
api_message_deserialise(char const *data)
{
	struct api_message *msg;
	int unsigned i, pos = 0;
	size_t dlen;

	msg = smalloc(sizeof(struct api_message), "API message");
	msg->command = data[pos++];
	msg->property = data[pos++];
	msg->nint = api_nint(msg->property);
	msg->nstr = api_nstr(msg->property);
	if (msg->nint > 0) {
		msg->argv_int = scalloc(msg->nint, sizeof(int),
		                        "API message integer arguments");
		for (i = 0; i < msg->nint; ++i) {
			dlen = sizeof(int);
			memcpy(&msg->argv_int[i], data + pos, dlen);
			pos += (int unsigned) dlen;
		}
	}
	if (msg->nstr > 0) {
		msg->argv_str = scalloc(msg->nstr, sizeof(char *),
		                        "API message string arguments");
		for (i = 0; i < msg->nint; ++i) {
			dlen = strlen(data + pos) + 1;
			msg->argv_str[i] = smalloc(dlen,
			                         "API message string argument");
			strcpy(msg->argv_str[i], data + pos);
			pos += (int unsigned) dlen;
		}
	}
	return msg;
}

struct api_message *
api_message_new(enum api_command command, enum api_property property, ...)
{
	struct api_message *msg;
	va_list args;
	int unsigned i;
	char *c;

	msg = smalloc(sizeof(struct api_message), "API message");
	msg->command = command;
	msg->property = property;
	msg->nint = api_nint(property);
	msg->nstr = api_nstr(property);
	va_start(args, property);
	if (msg->nint > 0) {
		msg->argv_int = scalloc(msg->nint, sizeof(int),
		                        "API message integer arguments");
		for (i = 0; i < msg->nint; ++i)
			msg->argv_int[i] = va_arg(args, int);
	}
	if (msg->nstr > 0) {
		msg->argv_str = scalloc(msg->nstr, sizeof(char *),
		                        "API message string arguments");
		for (i = 0; i < msg->nint; ++i) {
			c = va_arg(args, char *);
			msg->argv_str[i] = smalloc(strlen(c) + 1,
			                         "API message string argument");
			strcpy(msg->argv_str[i], c);
		}
	}
	va_end(args);

	return msg;
}

size_t
api_message_serialise(struct api_message *msg, size_t buflen, char *buf)
{
	int unsigned i, pos = 0;
	size_t dlen;

	buf[pos++] = (char) msg->command;
	buf[pos++] = (char) msg->property;
	for (i = 0; i < msg->nint; ++i) {
		dlen = sizeof(int);
		if (pos + dlen > buflen)
			goto api_message_serialise_error;
		memcpy(buf + pos, &msg->argv_int[i], dlen);
		pos += (int unsigned) dlen;
	}
	for (i = 0; i < msg->nstr; ++i) {
		dlen = strlen(msg->argv_str[i]) + 1;
		if (pos + dlen > buflen)
			goto api_message_serialise_error;
		strncpy(buf + pos, msg->argv_str[i], dlen);
		pos += (int unsigned) dlen;
	}
	return pos;

 api_message_serialise_error:
	ERROR("Cannot serialise API message %02X,%02X: buffer too small",
	      msg->command, msg->property);
	return 0;
}
