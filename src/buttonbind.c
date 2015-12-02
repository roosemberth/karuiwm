#include "buttonbind.h"
#include "util.h"
#include <string.h>

void
buttonbind_delete(struct buttonbind *bb)
{
	if (bb->action->argtype == ARGTYPE_STRING)
		sfree(bb->arg.v);
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
