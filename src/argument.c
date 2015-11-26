#include "argument.h"
#include "util.h"
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>

int
argument_fromstring(union argument *arg, char const *str,
                    enum argument_type type)
{
	if (strlen(str) == 0)
		return (type == ARGTYPE_NONE) ? 0 : -1;

	switch (type) {
	case ARGTYPE_NONE:
		return -1; /* it's supposed to be empty */
	case ARGTYPE_CHARACTER:
		if (strlen(str) != 1) {
			if (strlen(str) != 3
			|| (str[0] != '\'' && str[0] != '"')
			|| str[0] != str[2])
				return -1;
			arg->c = str[1];
		} else {
			arg->c = str[0];
		}
		return 0;
	case ARGTYPE_INTEGER:
	case ARGTYPE_INTEGER_UNSIGNED:
	case ARGTYPE_INTEGER_LONG:
	case ARGTYPE_INTEGER_LONG_UNSIGNED:
		errno = 0;
		switch (type) {
		case ARGTYPE_INTEGER:
			arg->i = (int) strtol(str, NULL, 0);
			break;
		case ARGTYPE_INTEGER_UNSIGNED:
			arg->ui = (int unsigned) strtol(str, NULL, 0);
			break;
		case ARGTYPE_INTEGER_LONG:
			arg->l = (int long) strtol(str, NULL, 0);
			break;
		case ARGTYPE_INTEGER_LONG_UNSIGNED:
			arg->ul = (int long unsigned) strtol(str, NULL, 0);
			break;
		default:
			return -1; /* shouldn't happen */
		}
		return errno;
	case ARGTYPE_FLOATING:
		errno = 0;
		arg->f = strtof(str, NULL);
		return errno;
	case ARGTYPE_FLOATING_DOUBLE:
		errno = 0;
		arg->d = strtod(str, NULL);
		return errno;
	case ARGTYPE_DIRECTION:
		if (strcasecmp(str, "left") == 0)
			arg->i = LEFT;
		else if (strcasecmp(str, "right") == 0)
			arg->i = RIGHT;
		else if (strcasecmp(str, "up") == 0)
			arg->i = UP;
		else if (strcasecmp(str, "down") == 0)
			arg->i = DOWN;
		else
			return -1;
		return 0;
	case ARGTYPE_LIST_DIRECTION:
		if (strcasecmp(str, "next") == 0)
			arg->i = NEXT;
		else if (strncasecmp(str, "prev", 4) == 0)
			arg->i = PREV;
		else
			return -1;
		return 0;
	case ARGTYPE_STRING:
		arg->v = strdupf("%s", str);
		return 0;
	}
	return -1; /* shouldn't happen */
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
