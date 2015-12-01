#include "buttonbind.h"
#include "util.h"
#include "karuiwm.h"
#include <string.h>

void
buttonbind_delete(struct buttonbind *bb)
{
	if (bb->action->argtype == ARGTYPE_STRING)
		sfree(bb->arg.v);
	sfree(bb);
}

struct buttonbind *
buttonbind_fromstring(char const *buttonstr, char const *actionargstr)
{
	struct button *button;
	struct action *action;
	union argument arg;
	struct buttonbind *bb = NULL;
	char *actionstr, *argstr;
	int unsigned i;
	size_t actionargstrlen = strlen(actionargstr);

	/* split action:argument string */
	for (i = 0; i < actionargstrlen && actionargstr[i] != ':'; ++i);
	actionstr = strdupf("%s", actionargstr);
	actionstr[i] = '\0';
	argstr = (i == actionargstrlen) ? actionstr + i : actionstr + i + 1;

	/* search corresponding action */
	for (i = 0, action = actions;
	     i < nactions && strcmp(action->name, actionstr) != 0;
	     ++i, action = action->next);
	if (i == nactions) {
		WARN("action not found: `%s`", actionstr);
		goto buttonbind_fromstring_out;
	}

	/* extract argument */
	if (argstr == NULL && action->argtype != ARGTYPE_NONE) {
		WARN("action `%s` requires non-empty argument", action->name);
		goto buttonbind_fromstring_out;
	}
	if (argument_fromstring(&arg, argstr, action->argtype) < 0) {
		WARN("action `%s` requires argument of type %s, but `%s` given",
		    action->name, argument_typestring(action->argtype), argstr);
		goto buttonbind_fromstring_out;
	}

	/* extract button */
	button = button_fromstring(buttonstr);
	if (button == NULL) {
		WARN("invalid key sequence: `%s`", buttonstr);
		goto buttonbind_fromstring_out;
	}

	/* compose button binding */
	bb = buttonbind_new(button, action, arg);

 buttonbind_fromstring_out:
	sfree(actionstr);
	return bb;
}

struct buttonbind *
buttonbind_new(struct button *button, struct action *a, union argument arg)
{
	struct buttonbind *bb;

	bb = smalloc(sizeof(struct buttonbind), "button binding");
	bb->button = button;
	bb->arg = arg;
	bb->action = a;
	return bb;
}
