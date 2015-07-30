#ifndef _XRESOURCE_H
#define _XRESOURCE_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <X11/Xresource.h>

struct xresource {
	XrmDatabase xdb;
	char *name;
};

void xresources_delete(struct xresource *xr);
struct xresource *xresource_new(char const *name);
int xresource_query_boolean(struct xresource *xr, char const *key, bool def,
                            bool *ret);
int xresource_query_colour(struct xresource *xr, char const *key, uint32_t def,
                           uint32_t *ret);
int xresource_query_integer(struct xresource *xr, char const *key, int def,
                            int *ret);
int xresource_query_string(struct xresource *xr, char const *key,
                           char const *def, char *ret, size_t retlen);

#endif /* ndef _XRESOURCE_H */
