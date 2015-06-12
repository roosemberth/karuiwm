#ifndef _EVENT_H
#define _EVENT_H

#define BUFSIZE 256

#include <stdbool.h>
#include <stdlib.h>

enum event_type {
	EVENT_FOCUS_MONITOR    = 1<<0,
	EVENT_FOCUS_WORKSPACE  = 1<<1,
	EVENT_FOCUS_DESKTOP    = 1<<2,
	EVENT_FOCUS_CLIENT     = 1<<3,
	EVENT_LAYOUT           = 1<<4,
	EVENT_MFACT            = 1<<5,
	EVENT_NMASTER          = 1<<6,
	EVENT_URGENT_WORKSPACE = 1<<7,
	EVENT_URGENT_DESKTOP   = 1<<8,
	EVENT_URGENT_CLIENT    = 1<<9,
};

union event {
	enum event_type type;
	struct {
		enum event_type type;
		int monitor_index;
	} focus_monitor;
	struct {
		enum event_type type;
		int monitor_index;
		char workspace_name[BUFSIZE];
	} focus_workspace;
	struct {
		enum event_type type;
		int monitor_index;
		char desktop_name[BUFSIZE];
	} focus_desktop;
	struct {
		enum event_type type;
		int monitor_index;
		char client_name[BUFSIZE];
	} focus_client;
	struct {
		enum event_type type;
		int monitor_index;
		char layout_name[BUFSIZE];
	} layout;
	struct {
		enum event_type type;
		int monitor_index;
		size_t nmaster;
	} nmaster;
	struct {
		enum event_type type;
		int monitor_index;
		float mfact;
	} mfact;
	struct {
		enum event_type type;
		int monitor_index;
		char workspace_name[BUFSIZE];
	} urgent_workspace;
	struct {
		enum event_type type;
		int monitor_index;
		char desktop_name[BUFSIZE];
	} urgent_desktop;
	struct {
		enum event_type type;
		int monitor_index;
		char client_name[BUFSIZE];
	} urgent_client;
};

void event_set_subscription(enum event_type type,
                            void (*event_handler)(union event *ev),
                            bool subscribe);
void event_trigger(union event *ev);

#endif /* def _EVENT_H */
