#ifndef _KARUIBAR_KEY_H
#define _KARUIBAR_KEY_H

#include <X11/Xlib.h>

struct key {
	int unsigned mod;
	KeySym sym;
};

void key_delete(struct key *k);
struct key *key_new(int unsigned mod, KeySym sym);
struct key *key_fromstring(char const *keystr);

#endif /* ndef _KARUIBAR_KEY_H */
