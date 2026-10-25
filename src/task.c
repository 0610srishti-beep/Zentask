#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "task.h"

static int next_id = 1;

void trim_newline(char *s) {
    if (!s) return;
    size_t n = strlen(s);
    while (n > 0 && (s[n - 1] == '\n' || s[n - 1] == '\r')) {
        s[n - 1] = '\0';
        n--;
    }
}

Task *create_task(const char *title, const char *desc, int priority, const char *due) {
    Task *t = (Task *)malloc(sizeof(Task));
    if (!t) { perror("malloc"); exit(1); }

    t->id = next_id++;
    strncpy(t->title, title ? title : "", MAX_TITLE - 1); t->title[MAX_TITLE - 1] = '\0';
    strncpy(t->desc, desc ? desc : "", MAX_DESC - 1);   t->desc[MAX_DESC - 1] = '\0';
    t->priority = priority;
    strncpy(t->due, due ? due : "", MAX_DATE - 1);     t->due[MAX_DATE - 1] = '\0';
    t->prev = t->next = t->hash_next = NULL;
    return t;
}

/* Used by file_io when loading records that already carry an id, so ids
 * stay stable across save/load and new tasks never collide with them. */
void task_bump_next_id(int id) {
    if (id >= next_id) next_id = id + 1;
}
