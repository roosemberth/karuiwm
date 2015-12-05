#ifndef _KARUIBAR_INPUT_H
#define _KARUIBAR_INPUT_H

#include "client.h"
#include <X11/Xlib.h>

void input_grab_buttons(struct client *c);
void input_grab_keys(void);
void input_handle_buttonpress(int unsigned mod, int unsigned button);
void input_handle_keypress(int unsigned mod, KeySym key);
int input_init(void);
int unsigned input_parse_modstr(char const *modstr);
void input_term(void);

#endif /* ndef _KARUIBAR_INPUT_H */
