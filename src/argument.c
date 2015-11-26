#include "argument.h"

int
argument_fromstring(union argument *arg, char const *str,
                    enum argument_type type)
{
	/* TODO */
	(void) arg;
	(void) str;
	(void) type;
	return -1;
}

char const *
argument_typestring(enum argument_type type)
{
	char const *argument_type_names[] = {
		[ARGTYPE_NONE] = "ARGTYPE_NONE",
		[ARGTYPE_CHARACTER] = "ARGTYPE_CHARACTER",
		[ARGTYPE_INTEGER] = "ARGTYPE_INTEGER",
		[ARGTYPE_INTEGER_UNSIGNED] = "ARGTYPE_INTEGER_UNSIGNED",
		[ARGTYPE_INTEGER_LONG] = "ARGTYPE_INTEGER_LONG",
		[ARGTYPE_INTEGER_LONG_UNSIGNED] = "ARGTYPE_INTEGER_LONG_UNSIGNED",
		[ARGTYPE_FLOATING] = "ARGTYPE_FLOATING",
		[ARGTYPE_FLOATING_DOUBLE] = "ARGTYPE_FLOATING_DOUBLE",
		[ARGTYPE_DIRECTION] = "ARGTYPE_DIRECTION",
		[ARGTYPE_LIST_DIRECTION] = "ARGTYPE_LIST_DIRECTION",
		[ARGTYPE_STRING] = "ARGTYPE_STRING",
	};
	return argument_type_names[type];
}
