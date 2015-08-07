#ifndef _KARUIWM_XRESOURCES_H
#define _KARUIWM_XRESOURCES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <X11/Xresource.h>

int xresources_boolean(char const *key, bool def, bool *ret);
int xresources_colour(char const *key, uint32_t def, uint32_t *ret);
int xresources_floating(char const *key, float def, float *ret);
int xresources_init(char const *name);
int xresources_integer(char const *key, int def, int *ret);
int xresources_string(char const *key, char const *def, char *ret, size_t retlen);
void xresources_term(void);

#endif /* ndef _KARUIWM_XRESOURCES_H */
