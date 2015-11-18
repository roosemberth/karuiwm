#ifndef _KARUIWM_EVENT_H
#define _KARUIWM_EVENT_H

#define BUFSIZE 256

#include <stdbool.h>
#include <stdlib.h>
#include <X11/Xlib.h>

enum event_type {
	EVENT_PRESS_BUTTON,
	EVENT_PRESS_KEY,
	EVENT_REGISTER_BUTTON,
	EVENT_REGISTER_KEY,
	EVENT_GRAB_BUTTONS,
	EVENT_GRAB_KEYS,
	EVENT_FOCUS_MONITOR,
	EVENT_FOCUS_WORKSPACE,
	EVENT_FOCUS_DESKTOP,
	EVENT_FOCUS_CLIENT,
	EVENT_LAYOUT,
	EVENT_MFACT,
	EVENT_NMASTER,
	EVENT_URGENT_WORKSPACE,
	EVENT_URGENT_DESKTOP,
	EVENT_URGENT_CLIENT,
	EVENT_LAST
};

union event {
	enum event_type type;

	/* registered button is pressed */
	struct press_button {
		enum event_type type;
		int unsigned mod;
		int unsigned button;
	} press_button;

	/* registered key is pressed */
	struct press_key {
		enum event_type type;
		int unsigned mod;
		KeySym key;
	} press_key;

	/* ask for a button to be registered - core handles this */
	struct register_button {
		enum event_type type;
		int unsigned mod;
		int unsigned button;
		bool reg;
	} register_button;

	/* ask for a key to be registered - core handles this */
	struct register_key {
		enum event_type type;
		int unsigned mod;
		KeySym key;
		bool reg;
	} register_key;

	/* ask for a window to grab all buttons */
	struct grab_buttons {
		enum event_type type;
		Window win;
		bool grab;
	} grab_buttons;

	/* ask for a window to grab all keys */
	struct grab_keys {
		enum event_type type;
		Window win;
		bool grab;
	} grab_keys;

	/* focused monitor changes */
	struct focus_monitor {
		enum event_type type;
		int monitor_index;
	} focus_monitor;

	/* focused workspace changes */
	struct focus_workspace {
		enum event_type type;
		int monitor_index;
		char workspace_name[BUFSIZE];
	} focus_workspace;

	/* focused desktop changes */
	struct focus_desktop {
		enum event_type type;
		int monitor_index;
		char desktop_name[BUFSIZE];
	} focus_desktop;

	/* focused client changes */
	struct focus_client {
		enum event_type type;
		int monitor_index;
		char client_name[BUFSIZE];
	} focus_client;

	/* client layout on current desktop changes */
	struct layout {
		enum event_type type;
		int monitor_index;
		char layout_name[BUFSIZE];
	} layout;

	/* number of clients in the master area changes */
	struct nmaster {
		enum event_type type;
		int monitor_index;
		size_t nmaster;
	} nmaster;

	/* master area size factor changes */
	struct mfact {
		enum event_type type;
		int monitor_index;
		float mfact;
	} mfact;

	/* workspace on which URGENT hint on a client changes */
	struct urgent_workspace {
		enum event_type type;
		int monitor_index;
		char workspace_name[BUFSIZE];
		bool urgent;
	} urgent_workspace;

	/* desktop on which URGENT hint on a client changes */
	struct urgent_desktop {
		enum event_type type;
		int monitor_index;
		char desktop_name[BUFSIZE];
		bool urgent;
	} urgent_desktop;

	/* URGENT hint on a client changes */
	struct urgent_client {
		enum event_type type;
		int monitor_index;
		char client_name[BUFSIZE];
		bool urgent;
	} urgent_client;
};

void event_set_subscription(enum event_type type,
                            void (*event_handler)(union event *ev),
                            bool subscribe);
void event_trigger(union event *ev);

#endif /* ndef _KARUIWM_EVENT_H */
