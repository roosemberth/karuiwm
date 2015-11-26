#define _POSIX_SOURCE

#include "xresource.h"
#include "util.h"
#include "karuiwm.h"
#include "list.h"
#include <string.h>
#include <strings.h>
#include <X11/Xlib.h>
#include <errno.h>

void
xresource_delete(struct xresource *xr)
{
	sfree(xr->key);
	sfree(xr->value);
	sfree(xr);
}

struct xresource *
xresource_new(char const *prefix, char const *line)
{
	struct xresource *xr;
	int unsigned offset, length;
	char *key, *value;
	size_t prefixlen = strlen(prefix);

	/* key */
	for (offset = 0; line[offset] == ' ' || line[offset] == '\t'; ++offset);
	for (length = 0; line[offset + length] != ' '
	              && line[offset + length] != '\t'
	              && line[offset + length] != ':'; ++length);
	if (strncmp(line + offset, prefix, prefixlen) != 0)
		return NULL;
	length -= (int unsigned) prefixlen;
	if (length == 0) {
		WARN("invalid key for `%s`", line);
		return NULL;
	}
	key = smalloc(length + 1, "X resource key");
	strncpy(key, line + offset + prefixlen, length);
	key[length] = '\0';

	/* value */
	for (offset = 0; line[offset] != ':'; ++offset);
	++offset;
	for (; line[offset] == ' ' || line[offset] == '\t'; ++offset);
	length = (int unsigned) strlen(line) - offset;
	for (; (line[offset+length-1] == ' ' || line[offset+length-1] == '\t')
	     && length > 0; --length);
	if (length == 0) {
		WARN("invalid value for `%s`", line);
		sfree(key);
		return NULL;
	}
	value = smalloc(length + 1, "X resource value");
	strncpy(value, line + offset, length);
	value[length] = '\0';

	/* X resource */
	xr = smalloc(sizeof(struct xresource), "X resource");
	xr->key = key;
	xr->value = value;

	return xr;
}
