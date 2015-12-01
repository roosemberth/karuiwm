#ifndef _KARUIBAR_BUTTON_H
#define _KARUIBAR_BUTTON_H

struct button {
	int unsigned mod;
	int unsigned sym;
};

void button_delete(struct button *b);
struct button *button_fromstring(char const *buttonstr);
int unsigned button_mod_fromstring(char const *modstr);
struct button *button_new(int unsigned mod, int unsigned sym);

#endif /* ndef _KARUIBAR_BUTTON_H */
