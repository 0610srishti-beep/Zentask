#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include "task.h"

extern Task *head;
extern Task *tail;

/* Appends t to the end of the doubly linked list. O(1). */
void list_insert_tail(Task *t);

/* Unlinks t from the doubly linked list. O(1); does not free t. */
void list_remove(Task *t);

#endif /* LINKED_LIST_H */
