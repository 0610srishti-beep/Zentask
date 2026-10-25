#ifndef TASK_H
#define TASK_H

#define MAX_TITLE 128
#define MAX_DESC  512
#define MAX_DATE  32
#define HASH_SIZE 1031   /* prime, keeps chains short for typical task counts */
#define LINE_BUF  1024
#define DATA_FILE "zentasks.csv"

/*
 * A single task.
 *
 * Every task lives in three structures at once:
 *   - a doubly linked list (insertion order, for `list`)
 *   - a hash table keyed on id (O(1) average lookup, for `show`/`edit`/`delete`)
 *   - a min-heap keyed on priority (O(log n) access to the most urgent task)
 *
 * `prev`/`next` and `hash_next` are the intrusive links for the first two;
 * the heap instead stores Task* in a separate array (see heap.h).
 */
typedef struct Task {
    int id;
    char title[MAX_TITLE];
    char desc[MAX_DESC];
    int priority;                 /* lower value = more urgent */
    char due[MAX_DATE];
    struct Task *prev, *next;     /* doubly linked list */
    struct Task *hash_next;       /* separate-chaining hash bucket */
} Task;

/* Allocates and fills a new Task (id is assigned automatically). */
Task *create_task(const char *title, const char *desc, int priority, const char *due);

/* Strips a trailing \n/\r left by fgets(). */
void trim_newline(char *s);

/* Advances the auto-increment id counter past `id` (used when reloading
 * records from disk so freshly added tasks never reuse an id). */
void task_bump_next_id(int id);

#endif /* TASK_H */
