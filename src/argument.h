#ifndef _KARUIBAR_ARGUMENT_H
#define _KARUIBAR_ARGUMENT_H

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

#endif /* ndef _KARUIBAR_ARGUMENT_H */
