#include "key.h"
#include "util.h"

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
	/* TODO */
	(void) keystr;
	return NULL;
}
