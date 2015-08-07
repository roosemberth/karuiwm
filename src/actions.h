#ifndef _KARUIWM_ACTIONS_H
#define _KARUIWM_ACTIONS_H

#include <X11/Xlib.h>

union argument {
	int i;
	float f;
	void *v;
};

void action_killclient(union argument *arg);
void action_mousemove(union argument *arg, Window win);
void action_mouseresize(union argument *arg, Window win);
void action_restart(union argument *arg);
void action_setmfact(union argument *arg);
void action_setnmaster(union argument *arg);
void action_shiftclient(union argument *arg);
void action_spawn(union argument *arg);
void action_stepclient(union argument *arg);
void action_stepdesktop(union argument *arg);
void action_steplayout(union argument *arg);
void action_stop(union argument *arg);
void action_togglefloat(union argument *arg);
void action_zoom(union argument *arg);

#endif /* ndef _KARUIWM_ACTIONS_H */
