#include "layout.h"
#include "util.h"

void
layout_delete(struct layout *l)
{
	sfree(l);
}

struct layout *
layout_new(char const *name, layout_func apply)
{
	struct layout *l;

	l = smalloc(sizeof(struct layout), "layout");
	l->apply = apply;
	l->name = strdupf("%s", name);
	return l;
}
