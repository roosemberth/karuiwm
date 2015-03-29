#include "layout.h"
#include "util.h"
#include "list.h"
#include <string.h>

#include "layouts/rstack.h"
#include "layouts/monocle.h"

static void layout_delete(struct layout *l);
static struct layout *layout_new(layout_func apply, char const *name);

static void
layout_delete(struct layout *l)
{
	free(l);
}

void
layout_init(void)
{
	struct layout *l;

	layouts = NULL;

	/* TODO more modular layouts, this is ugly as hell */
	l = layout_new(rstack, "rstack");
	LIST_APPEND(&layouts, l);
	l = layout_new(monocle, "monocle");
	LIST_APPEND(&layouts, l);
}

static struct layout *
layout_new(layout_func apply, char const *name)
{
	struct layout *l;

	l = smalloc(sizeof(struct layout), "layout");
	l->apply = apply;
	strncpy(l->name, name, BUFSIZ);
	l->name[BUFSIZ - 1] = '\0';
	return l;
}

void
layout_term(void)
{
	struct layout *l;

	while (layouts != NULL) {
		l = layouts;
		LIST_REMOVE(&layouts, l);
		layout_delete(l);
	}
}
