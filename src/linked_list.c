#include <stddef.h>
#include "linked_list.h"

Task *head = NULL;
Task *tail = NULL;

void list_insert_tail(Task *t) {
    if (!tail) {
        head = tail = t;
    } else {
        tail->next = t;
        t->prev = tail;
        tail = t;
    }
}

void list_remove(Task *t) {
    if (!t) return;
    if (t->prev) t->prev->next = t->next;
    else head = t->next;
    if (t->next) t->next->prev = t->prev;
    else tail = t->prev;
    t->prev = t->next = NULL;
}
