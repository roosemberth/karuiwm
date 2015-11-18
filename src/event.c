#include "event.h"
#include "util.h"
#include "list.h"

struct subscription {
	struct subscription *prev, *next;
	void (*event_handler)(union event *ev);
};

static struct subscription *findsub(void (*event_handler)(union event *ev),
                                    enum event_type type);

static struct subscription *subs[EVENT_LAST] = { NULL };
static size_t nsubs[EVENT_LAST] = { 0 };

void
event_set_subscription(enum event_type type,
                       void (*event_handler)(union event *ev), bool subscribe)
{
	struct subscription *s;

	if (type >= EVENT_LAST) {
		WARN("subscription attempt for invalid ID %d", type);
		return;
	}

	s = findsub(event_handler, type);
	if (subscribe && s == NULL) {
		s = smalloc(sizeof(struct subscription), "subscription");
		s->event_handler = event_handler;
		LIST_APPEND(subs[type], s);
		++nsubs[type];
	} else if (!subscribe && s != NULL) {
		LIST_REMOVE(subs[type], s);
		--nsubs[type];
		sfree(s);
	}
}

void
event_trigger(union event *ev)
{
	int unsigned i;
	struct subscription *s;

	for (i = 0, s = subs[ev->type]; i < nsubs[ev->type]; ++i, s = s->next)
		s->event_handler(ev);
}

static struct subscription *
findsub(void (*event_handler)(union event *ev), enum event_type type)
{
	int unsigned i;
	struct subscription *s;

	for (i = 0, s = subs[type]; i < nsubs[type]; ++i, s = s->next)
		if (s->event_handler == event_handler)
			return s;
	return NULL;
}
