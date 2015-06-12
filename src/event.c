#include "event.h"
#include <util.h>

struct subscription {
	void (*event_handler)(union event *ev);
	enum event_type event_mask;
};

static struct subscription *find_subscription(void (*event_handler)(union event *ev));

static struct subscription *subscriptions = NULL;
static size_t ns = 0;

void
event_set_subscription(enum event_type type,
                       void (*event_handler)(union event *ev), bool subscribe)
{
	int unsigned i;
	struct subscription *sub = find_subscription(event_handler);

	if (subscribe) {
		if (sub == NULL) {
			++ns;
			subscriptions = srealloc(subscriptions,
			                         ns*sizeof(struct subscription),
			                         "event subscription list");
			sub = &subscriptions[ns - 1];
			sub->event_handler = event_handler;
			sub->event_mask = type;
		} else {
			sub->event_mask |= type;
		}
	} else if (sub != NULL) {
		sub->event_mask &= ~type;
		if (sub->event_mask == 0) {
			for (i = 0; i < ns && &subscriptions[i] != sub; ++i);
			--ns;
			for (; i < ns; ++i)
				subscriptions[i] = subscriptions[i + 1];
			subscriptions = srealloc(subscriptions,
			                         ns*sizeof(struct subscription),
			                         "event subscription list");
		}
	}
}

void
event_trigger(union event *ev)
{
	int unsigned i;

	for (i = 0; i < ns; ++i)
		if (ev->type & subscriptions[i].event_mask != 0)
			subscriptions[i].event_handler(ev);
}

static struct subscription *
find_subscription(void (*event_handler)(union event *ev))
{
	int unsigned i;

	for (i = 0; i < ns; ++i)
		if (subscriptions[i].event_handler == event_handler)
			return &subscriptions[i];
	return NULL;
}
