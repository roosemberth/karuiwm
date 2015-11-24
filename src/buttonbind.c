#include "buttonbind.h"
#include "util.h"

void
buttonbind_delete(struct buttonbind *bb)
{
	sfree(bb);
}

struct buttonbind *
buttonbind_new(int unsigned mod, int unsigned button, struct action *a,
               union argument arg)
{
	struct buttonbind *bb;

	bb = smalloc(sizeof(struct buttonbind), "button binding");
	bb->mod = mod;
	bb->button = button;
	bb->arg = arg;
	bb->action = a;
	return bb;
}

struct buttonbind *
buttonbind_new_fromstring(char const *buttonstr, struct action *a,
                          union argument arg)
{
	/* TODO */
	(void) buttonstr;
	(void) a;
	(void) arg;
	return NULL;
}
