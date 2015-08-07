#ifndef _KARUIWM_LIST_H
#define _KARUIWM_LIST_H

#define LIST_APPEND(H, E) list_append((struct list_element **) (H), \
                                      (struct list_element *) (E))
#define LIST_PREPEND(H, E) list_prepend((struct list_element **) (H), \
                                        (struct list_element *) (E))
#define LIST_REMOVE(H, E) list_remove((struct list_element **) (H), \
                                      (struct list_element *) (E))

struct list_element {
	struct list_element *prev, *next;
};

void list_append(struct list_element **head, struct list_element *e);
struct list_element *list_neighbour(struct list_element *e, int dir);
void list_prepend(struct list_element **head, struct list_element *e);
void list_remove(struct list_element **head, struct list_element *e);
void list_swap(struct list_element **head, struct list_element *e1,
                                           struct list_element *e2);

#endif /* ndef _KARUIWM_LIST_H */
