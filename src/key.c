#include "key.h"
#include "util.h"
#include "config.h"
#include <string.h>

void
key_delete(struct key *k)
{
	sfree(k);
}

struct key *
key_new(int unsigned mod, KeySym sym)
{
	struct key *k;

	k = smalloc(sizeof(struct key), "key");
	k->mod = mod;
	k->sym = sym;
	return k;
}

struct key *
key_fromstring(char const *keystr)
{
	struct key *key;
	int unsigned sympos;
	KeySym sym;
	size_t keystrlen = strlen(keystr);
	char *modstr;
	char const *modtok;
	int unsigned mod = 0;

	/* find XK_ part */
	for (sympos = (int unsigned) keystrlen;
	     sympos > 0 && keystr[sympos - 1] != '-';
	     --sympos);
	sym = XStringToKeysym(keystr + sympos);
	if (sym == NoSymbol) {
		WARN("symbol not found for `%s`", keystr + sympos);
		return NULL;
	}

	/* extract modifiers */
	if (sympos > 0) {
		modstr = strdupf("%s", keystr);
		modtok = strtok(modstr, "-");
		do {
			if (modtok == modstr + sympos)
				break;
			mod |= key_mod_fromstring(modtok);
		} while ((modtok = strtok(NULL, "-")) != NULL);
		sfree(modstr);
	}

	key = key_new(mod, sym);
	return key;
}

int unsigned
key_mod_fromstring(char const *modstr)
{
	switch (modstr[0]) {
	case 'M': return config.modifier;
	case 'S': return ShiftMask;
	case 'C': return ControlMask;
	case 'W': return Mod4Mask;
	case 'A': return Mod1Mask;
	default:
		WARN("unknown modifier: `%s`", modstr);
		return 0;
	}
}
