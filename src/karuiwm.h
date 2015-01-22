#ifndef _KARUIWM_H
#define _KARUIWM_H

#include "client.h"
#include <X11/Xlib.h>
#include <stdbool.h>
#include <stdio.h>

/* macros */
#define BORDERWIDTH 1    /* window border width */
#define LENGTH(ARR) (sizeof(ARR)/sizeof(ARR[0]))
#define MAX(X, Y) ((X) < (Y) ? (Y) : (X))
#define MIN(X, Y) ((X) < (Y) ? (X) : (Y))
#define DIE(...)    {print(stderr, LOG_FATAL,   __FILE__, __LINE__, __VA_ARGS__);\
                     exit(EXIT_FAILURE);}
#define ERROR(...)   print(stderr, LOG_ERROR,   __FILE__, __LINE__, __VA_ARGS__)
#define WARN(...)    print(stderr, LOG_WARN,    __FILE__, __LINE__, __VA_ARGS__)
#define EVENT(...)   print(stderr, LOG_EVENT,   __FILE__, __LINE__, __VA_ARGS__)
#define NOTICE(...)  print(stdout, LOG_NOTICE,  __FILE__, __LINE__, __VA_ARGS__)
#define NORMAL(...)  print(stdout, LOG_NORMAL,  __FILE__, __LINE__, __VA_ARGS__)
#define VERBOSE(...) print(stdout, LOG_VERBOSE, __FILE__, __LINE__, __VA_ARGS__)
#define DEBUG(...)   print(stdout, LOG_DEBUG,   __FILE__, __LINE__, __VA_ARGS__)

/* enumerations */
enum log_level { LOG_FATAL, LOG_ERROR, LOG_WARN, LOG_NOTICE, LOG_NORMAL,
                 LOG_VERBOSE, LOG_EVENT, LOG_DEBUG };
enum { WMProtocols, WMDeleteWindow, WMState, WMTakeFocus, WMLAST };
enum { NetActiveWindow, NetSupported, NetWMName, NetWMState,
       NetWMStateFullscreen, NetWMWindowType, NetWMWindowTypeDialog, NetLAST };

/* structures/unions */
union argument {
	int i;
	float f;
	void *v;
};

struct button {
	int unsigned mod;
	int unsigned button;
	void (*func)(union argument const *);
	union argument const arg;
};

struct key {
	int unsigned mod;
	KeySym key;
	void (*func)(union argument const *);
	union argument const arg;
};

struct {
	Display *dpy;
} kwm;

/* functions */
void grabbuttons(struct client *c, bool grab);
void moveresizeclient(struct client *c, int x, int y, int unsigned w, int unsigned h);
void print(FILE *f, enum log_level level, char const *filename,
           int unsigned line, char const *format, ...);
void setclientmask(bool);

/* variables */
Atom wmatom[WMLAST], netatom[NetLAST];

#endif /* _KARUIWM_H */
