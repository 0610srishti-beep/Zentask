#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "commands.h"
#include "task.h"
#include "linked_list.h"
#include "hash_table.h"
#include "priority_heap.h"

void cmd_add(void) {
    char title[MAX_TITLE], desc[MAX_DESC], due[MAX_DATE], buf[LINE_BUF];
    int priority;

    printf("Title: ");
    if (!fgets(title, sizeof title, stdin)) return;
    trim_newline(title);
    if (strlen(title) == 0) { printf("Title cannot be empty.\n"); return; }

    printf("Description: ");
    if (!fgets(desc, sizeof desc, stdin)) return;
    trim_newline(desc);

    printf("Priority (integer, lower = more urgent): ");
    if (!fgets(buf, sizeof buf, stdin)) return;
    priority = atoi(buf);
    if (priority < 0) priority = 0;

    printf("Due (text, e.g. 2025-12-31 or 'none'): ");
    if (!fgets(due, sizeof due, stdin)) return;
    trim_newline(due);

    Task *t = create_task(title, desc, priority, due);
    list_insert_tail(t);
    hash_insert(t);
    heap_insert(t);

    printf("Added task id %d\n", t->id);
}

void cmd_list(void) {
    if (!head) { printf("No tasks.\n"); return; }
    printf("ID\tPri\tDue\tTitle\n");
    printf("-----------------------------------------------\n");
    for (Task *p = head; p; p = p->next) {
        printf("%d\t%d\t%s\t%s\n", p->id, p->priority, p->due[0] ? p->due : "-", p->title);
    }
}

void cmd_show(int id) {
    Task *t = hash_find(id);
    if (!t) { printf("Task %d not found.\n", id); return; }
    printf("ID: %d\nTitle: %s\nDescription: %s\nPriority: %d\nDue: %s\n",
           t->id, t->title, t->desc, t->priority, t->due[0] ? t->due : "-");
}

void cmd_top(void) {
    Task *t = heap_top();
    if (!t) { printf("No tasks.\n"); return; }
    printf("Top task (id %d, priority %d): %s\n", t->id, t->priority, t->title);
}

void cmd_delete(int id) {
    Task *t = hash_find(id);
    if (!t) { printf("Task %d not found.\n", id); return; }
    list_remove(t);
    hash_remove(t);
    heap_remove(t);
    free(t);
    printf("Deleted task %d\n", id);
}

void cmd_edit(int id) {
    Task *t = hash_find(id);
    if (!t) { printf("Task %d not found.\n", id); return; }
    char buf[LINE_BUF], tmp[MAX_DESC];
    printf("Editing task %d. Press enter to keep current value.\n", id);

    printf("Title [%s]: ", t->title);
    if (!fgets(buf, sizeof buf, stdin)) return;
    trim_newline(buf);
    if (strlen(buf)) { strncpy(t->title, buf, MAX_TITLE - 1); t->title[MAX_TITLE - 1] = '\0'; }

    printf("Description [%s]: ", t->desc);
    if (!fgets(tmp, sizeof tmp, stdin)) return;
    trim_newline(tmp);
    if (strlen(tmp)) { strncpy(t->desc, tmp, MAX_DESC - 1); t->desc[MAX_DESC - 1] = '\0'; }

    printf("Priority [%d]: ", t->priority);
    if (!fgets(buf, sizeof buf, stdin)) return;
    trim_newline(buf);
    if (strlen(buf)) {
        int newp = atoi(buf);
        if (newp != t->priority) {
            /* priority changed -> must re-sift the task's heap position */
            heap_remove(t);
            t->priority = newp;
            heap_insert(t);
        }
    }

    printf("Due [%s]: ", t->due);
    if (!fgets(tmp, sizeof tmp, stdin)) return;
    trim_newline(tmp);
    if (strlen(tmp)) { strncpy(t->due, tmp, MAX_DATE - 1); t->due[MAX_DATE - 1] = '\0'; }

    printf("Task updated.\n");
}

void print_help(void) {
    printf("Commands:\n");
    printf("  add            - Add a new task\n");
    printf("  list           - List tasks\n");
    printf("  show <id>      - Show full info for task id\n");
    printf("  top            - Show highest priority task\n");
    printf("  delete <id>    - Delete task by id\n");
    printf("  edit <id>      - Edit task fields\n");
    printf("  save [file]    - Save tasks to CSV (default zentasks.csv)\n");
    printf("  load [file]    - Load tasks from CSV (replaces current in memory)\n");
    printf("  help           - Show this help\n");
    printf("  exit           - Save & Exit (or just exit to quit without saving)\n");
}

void cleanup_all(void) {
    Task *p = head;
    while (p) {
        Task *n = p->next;
        free(p);
        p = n;
    }
    head = tail = NULL;
    hash_clear();
    heap_destroy();
}
