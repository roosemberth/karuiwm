#ifndef _KARUIWM_XRESOURCES_H
#define _KARUIWM_XRESOURCES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <X11/Xresource.h>

struct xresource {
	struct xresource *prev, *next;
	char *key;
	char *value;
};

void xresource_delete(struct xresource *xr);
struct xresource *xresource_new(char const *prefix, char const *line);

#endif /* ndef _KARUIWM_XRESOURCES_H */
