#include "list.h"
#include <stdlib.h>

void
list_append(struct list_element **head, struct list_element *e)
{
	if (*head == NULL) {
		e->prev = e->next = e;
		*head = e;
	} else {
		e->prev = (*head)->prev;
		e->next = *head;
		e->prev->next = e->next->prev = e;
	}
}

inline struct list_element *
list_neighbour(struct list_element *e, int dir)
{
	return e == NULL ? NULL : dir < 0 ? e->prev : e->next;
}

void
list_prepend(struct list_element **head, struct list_element *e)
{
	if (*head == NULL) {
		e->prev = e->next = e;
	} else {
		e->prev = (*head)->prev;
		e->next = *head;
		e->prev->next = e->next->prev = e;
	}
	*head = e;
}

void
list_remove(struct list_element **head, struct list_element *e)
{
	if (e->next == e) {
		*head = NULL;
	} else {
		if (e == *head)
			*head = e->next;
		e->prev->next = e->next;
		e->next->prev = e->prev;
	}
	e->prev = e->next = NULL;
}

size_t
list_size(struct list_element *head)
{
	struct list_element *e;
	size_t n;

	if (head == NULL)
		return 0;

	for (e = head->next, n = 1; e != head; e = e->next, ++n);
	return n;
}

void
list_swap(struct list_element **head, struct list_element *e1,
                                      struct list_element *e2)
{
	struct list_element *prev1, *next1, *prev2, *next2;

	if (e1 == NULL || e2 == NULL)
		return;

	prev1 = e1->prev;
	next1 = e1->next;
	prev2 = e2->prev;
	next2 = e2->next;

	if (e1 == e2) {
		/* same node */
	} else if (e1->next == e2 && e2->next == e1) {
		/* only two nodes in list */
	} else if (e1->next == e2) {
		/* next to each other */
		prev1->next = e2;
		e2->next = e1;
		e1->next = next2;
		next2->prev = e1;
		e1->prev = e2;
		e2->prev = prev1;
	} else if (e2->next == e1) {
		/* next to each other (alt.) */
		list_swap(head, e2, e1);
		return;
	} else {
		/* general case */
		prev1->next = e2;
		e2->next = next1;
		next1->prev = e2;
		e2->prev = prev1;
		prev2->next = e1;
		e1->next = next2;
		next2->prev = e1;
		e1->prev = prev2;
	}

	/* fix list head */
	if (*head == e1)
		*head = e2;
	else if (*head == e2)
		*head = e1;
}
