#include "action.h"
#include "util.h"

void
action_delete(struct action *a)
{
	sfree(a->name);
	sfree(a);
}

struct action *
action_new(char const *name, void (*function)(union argument *),
           enum argument_type argtype)
{
	struct action *a;

	a = smalloc(sizeof(struct action), "action");
	a->name = strdupf(name);
	a->function = function;
	a->argtype = argtype;
	return a;
}
