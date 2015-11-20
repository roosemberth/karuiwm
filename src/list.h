#ifndef _KARUIWM_LIST_H
#define _KARUIWM_LIST_H

#include <stdlib.h>

#define LIST_APPEND(H, E) list_append((struct list_element **) (H), \
                                      (struct list_element *) (E))
#define LIST_PREPEND(H, E) list_prepend((struct list_element **) (H), \
                                        (struct list_element *) (E))
#define LIST_REMOVE(H, E) list_remove((struct list_element **) (H), \
                                      (struct list_element *) (E))
#define LIST_SIZE(H) list_size((struct list_element *) (H));
#define LIST_SWAP(H, E1, E2) list_swap((struct list_element **) (H), \
                                       (struct list_element *) (E1), \
                                       (struct list_element *) (E2))

struct list_element {
	struct list_element *prev, *next;
};

void list_append(struct list_element **head, struct list_element *e);
struct list_element *list_neighbour(struct list_element *e, int dir);
void list_prepend(struct list_element **head, struct list_element *e);
void list_remove(struct list_element **head, struct list_element *e);
size_t list_size(struct list_element *head);
void list_swap(struct list_element **head, struct list_element *e1,
                                           struct list_element *e2);

#endif /* ndef _KARUIWM_LIST_H */
