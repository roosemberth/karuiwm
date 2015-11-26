#ifndef _KARUIBAR_KEYBIND_H
#define _KARUIBAR_KEYBIND_H

#include "key.h"
#include "action.h"
#include "argument.h"

struct keybind {
	struct keybind *prev, *next;
	struct key *key;
	struct action *action;
	union argument arg;
};

void keybind_delete(struct keybind *kb);
struct keybind *keybind_new(struct key *key, struct action *action,
                            union argument arg);
struct keybind *keybind_fromstring(char const *keystr,
                                   char const *actionargstr);

#endif /* ndef _KARUIBAR_KEYBIND_H */
