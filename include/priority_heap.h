#ifndef PRIORITY_HEAP_H
#define PRIORITY_HEAP_H

#include "task.h"

/*
 * A binary min-heap of Task* ordered by priority (ties broken by id),
 * giving O(log n) insert/remove and O(1) access to the most urgent task.
 */

/* Inserts t into the heap. O(log n). */
void heap_insert(Task *t);

/* Removes t from anywhere in the heap. O(n) to locate + O(log n) to resift. */
void heap_remove(Task *t);

/* Returns the highest-priority task without removing it, or NULL if empty. */
Task *heap_top(void);

/* Frees the heap's backing array and resets its size. */
void heap_destroy(void);

/* Resets the heap to empty without freeing the backing array. */
void heap_clear(void);

#endif /* PRIORITY_HEAP_H */
