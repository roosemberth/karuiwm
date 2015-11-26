#ifndef _KARUIWM_CONFIG_H
#define _KARUIWM_CONFIG_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

struct {
	struct {
		uint32_t colour, colour_focus;
		int unsigned width;
	} border;
	int unsigned modifier;
} config;

int config_get_bool(char const *key, bool def, bool *ret);
int config_get_colour(char const *key, uint32_t def, uint32_t *ret);
int config_get_float(char const *key, float def, float *ret);
int config_get_int(char const *key, int def, int *ret);
struct keybind *config_get_keybinds(void);
int config_get_string(char const *key, char const *def, char *ret, size_t retlen);
int config_init(void);
void config_set_bufsize(size_t size);
void config_term(void);

#endif /* ndef _KARUIWM_CONFIG_H */
