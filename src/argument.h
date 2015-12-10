#ifndef _KARUIBAR_ARGUMENT_H
#define _KARUIBAR_ARGUMENT_H

enum direction { UP, DOWN, LEFT, RIGHT };
enum list_direction { PREV, NEXT };
enum argument_type {
	ARGTYPE_NONE,
	ARGTYPE_CHARACTER,
	ARGTYPE_INTEGER,
	ARGTYPE_INTEGER_UNSIGNED,
	ARGTYPE_INTEGER_LONG,
	ARGTYPE_INTEGER_LONG_UNSIGNED,
	ARGTYPE_FLOATING,
	ARGTYPE_FLOATING_DOUBLE,
	ARGTYPE_DIRECTION,      /* up, down, left, right */
	ARGTYPE_LIST_DIRECTION, /* next, prev */
	ARGTYPE_STRING,
	ARGTYPE_MOUSE,
	ARGTYPE_LAYOUT,
};

union argument {
	char c;
	int i;
	int unsigned ui;
	int long l;
	int long unsigned ul;
	float f;
	double d;
	void *v;
};

int argument_fromstring(union argument *arg, char const *str,
                        enum argument_type type);
char const *argument_typestring(enum argument_type type);

#endif /* ndef _KARUIBAR_ARGUMENT_H */
