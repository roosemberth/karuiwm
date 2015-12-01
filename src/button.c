#include "button.h"
#include "util.h"
#include "config.h"
#include <string.h>
#include <X11/Xlib.h>

static int unsigned sym_fromstring(char const *str);

void
button_delete(struct button *b)
{
	sfree(b);
}

struct button *
button_fromstring(char const *buttonstr)
{
	int unsigned sympos;
	int unsigned sym;
	size_t buttonstrlen = strlen(buttonstr);
	char *modstr;
	char const *modtok;
	int unsigned mod;

	/* find Button part */
	for (sympos = (int unsigned) buttonstrlen;
	     sympos > 0 && buttonstr[sympos - 1] != '-';
	     --sympos);
	sym = sym_fromstring(buttonstr + sympos);
	if (sym == 0) {
		WARN("button symbol not found for `%s`", buttonstr + sympos);
		return NULL;
	}

	/* extract modifiers */
	mod = 0;
	if (sympos > 0) {
		modstr = strdupf("%s", buttonstr);
		modtok = strtok(modstr, "-");
		do {
			if (modtok == modstr + sympos)
				break;
			mod |= button_mod_fromstring(modtok);
		} while ((modtok = strtok(NULL, "-")) != NULL);
		sfree(modstr);
	}

	return button_new(mod, sym);
}

int unsigned
button_mod_fromstring(char const *modstr)
{
	switch (modstr[0]) {
	case 'M': return config.modifier;
	case 'S': return ShiftMask;
	case 'C': return ControlMask;
	case 'W': return Mod4Mask;
	case 'A': return Mod1Mask;
	default:
		WARN("unknown modifier: `%s`", modstr);
		return 0;
	}
}

struct button *
button_new(int unsigned mod, int unsigned sym)
{
	struct button *b;

	b = smalloc(sizeof(struct button), "button");
	b->mod = mod;
	b->sym = sym;
	return b;
}

static int unsigned
sym_fromstring(char const *str)
{
	char index;

	if (strlen(str) != 1)
		return 0;
	index = str[0];
	index = (char) ((int) index - 48);

	/* TODO here we only accept buttons 1-5 */
	return (index > 0 && index <= 5) ? (int unsigned) index : 0;
}
