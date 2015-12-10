#include "argument.h"
#include "util.h"
#include "api.h"
#include <string.h>
#include <strings.h>
#include <errno.h>
#include <stdlib.h>

int
argument_fromstring(union argument *arg, char const *str,
                    enum argument_type type)
{
	int unsigned i;
	struct layout *layout;

	switch (type) {
	case ARGTYPE_NONE:
	case ARGTYPE_MOUSE:
		return -((int signed) strlen(str));
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
	case ARGTYPE_LAYOUT:
		for (i = 0, layout = api.layouts;
		     i < api.nlayouts && strcmp(str, layout->name) != 0;
		     ++i, layout = layout->next);
		if (i == api.nlayouts)
			return -1;
		arg->v = layout;
		return 0;
	default:
		WARN("attempt to parse unknown argument type %d", type);
		return -1;
	}
}

char const *
argument_typestring(enum argument_type type)
{
	char const *typestrings[] = {
		[ARGTYPE_NONE] = "NONE",
		[ARGTYPE_CHARACTER] = "CHARACTER",
		[ARGTYPE_INTEGER] = "INTEGER",
		[ARGTYPE_INTEGER_UNSIGNED] = "INTEGER_UNSIGNED",
		[ARGTYPE_INTEGER_LONG] = "INTEGER_LONG",
		[ARGTYPE_INTEGER_LONG_UNSIGNED] = "INTEGER_LONG_UNSIGNED",
		[ARGTYPE_FLOATING] = "FLOATING",
		[ARGTYPE_FLOATING_DOUBLE] = "FLOATING_DOUBLE",
		[ARGTYPE_DIRECTION] = "DIRECTION",
		[ARGTYPE_LIST_DIRECTION] = "LIST_DIRECTION",
		[ARGTYPE_STRING] = "STRING",
		[ARGTYPE_MOUSE] = "MOUSE",
		[ARGTYPE_LAYOUT] = "ARGTYPE_LAYOUT",
	};
	if (type >= sizeof(typestrings) / sizeof(typestrings[0])) {
		WARN("attempt to stringify unknown argument type %d", type);
		return NULL;
	}
	return typestrings[type];
}
