#include <stddef.h>
#include "hash_table.h"

Task *hash_table[HASH_SIZE];

static unsigned int hash_fn(int id) {
    return (unsigned int)id % HASH_SIZE;
}

void hash_insert(Task *t) {
    unsigned int idx = hash_fn(t->id);
    t->hash_next = hash_table[idx];
    hash_table[idx] = t;
}

Task *hash_find(int id) {
    unsigned int idx = hash_fn(id);
    Task *p = hash_table[idx];
    while (p) {
        if (p->id == id) return p;
        p = p->hash_next;
    }
    return NULL;
}

void hash_remove(Task *t) {
    unsigned int idx = hash_fn(t->id);
    Task *p = hash_table[idx];
    Task *prev = NULL;
    while (p) {
        if (p == t) {
            if (prev) prev->hash_next = p->hash_next;
            else hash_table[idx] = p->hash_next;
            t->hash_next = NULL;
            return;
        }
        prev = p;
        p = p->hash_next;
    }
}

void hash_clear(void) {
    for (int i = 0; i < HASH_SIZE; ++i) hash_table[i] = NULL;
}
