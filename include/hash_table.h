#ifndef HASH_TABLE_H
#define HASH_TABLE_H

#include "task.h"

extern Task *hash_table[HASH_SIZE];

/* Inserts t into the hash table, chained on id % HASH_SIZE. O(1) average. */
void hash_insert(Task *t);

/* Finds the task with the given id, or NULL. O(1) average. */
Task *hash_find(int id);

/* Removes t from its hash chain. O(chain length); does not free t. */
void hash_remove(Task *t);

/* Resets every bucket to NULL (used when reloading from file). */
void hash_clear(void);

#endif /* HASH_TABLE_H */
