#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "file_io.h"
#include "task.h"
#include "linked_list.h"
#include "hash_table.h"
#include "priority_heap.h"

/* Writes `src` into `dst` wrapped in double quotes, doubling any embedded
 * quote (the standard CSV escape), so commas/quotes in free text are safe. */
static void csv_escape(char *dst, size_t dst_size, const char *src) {
    size_t j = 0;
    dst[j++] = '"';
    for (size_t i = 0; src[i] && j < dst_size - 3; ++i) {
        if (src[i] == '"') { dst[j++] = '"'; dst[j++] = '"'; }
        else dst[j++] = src[i];
    }
    dst[j++] = '"';
    dst[j] = '\0';
}

void save_tasks(const char *filename) {
    FILE *f = fopen(filename, "w");
    if (!f) { perror("fopen"); return; }

    fprintf(f, "id,title,description,priority,due\n");
    for (Task *p = head; p; p = p->next) {
        char title[MAX_TITLE * 2], desc[MAX_DESC * 2], due[MAX_DATE * 2];
        csv_escape(title, sizeof title, p->title);
        csv_escape(desc, sizeof desc, p->desc);
        csv_escape(due, sizeof due, p->due);
        fprintf(f, "%d,%s,%s,%d,%s\n", p->id, title, desc, p->priority, due);
    }

    fclose(f);
    printf("Saved to %s\n", filename);
}

/* Splits one CSV line into up to 5 fields, honoring double-quoted fields
 * with "" as an escaped quote. Caller must free every non-NULL field. */
static int csv_parse_line(char *line, char *fields[5]) {
    char *s = line;
    int fld = 0;

    while (*s && fld < 5) {
        if (*s == '"') {
            s++;
            char tmp[LINE_BUF];
            int ti = 0;
            while (*s) {
                if (*s == '"' && s[1] == '"') { tmp[ti++] = '"'; s += 2; continue; }
                if (*s == '"') { s++; break; }
                tmp[ti++] = *s++;
            }
            tmp[ti] = '\0';
            fields[fld++] = strdup(tmp);
        } else {
            char *start = s;
            while (*s && *s != ',') s++;
            int len = (int)(s - start);
            char *token = (char *)malloc(len + 1);
            strncpy(token, start, len);
            token[len] = '\0';
            fields[fld++] = token;
        }
        if (*s == ',') s++;
    }
    return fld;
}

static void clear_all_tasks(void) {
    Task *p = head;
    while (p) {
        Task *n = p->next;
        free(p);
        p = n;
    }
    head = tail = NULL;
    hash_clear();
    heap_clear();
}

int load_tasks(const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) { perror("fopen"); return 0; }

    clear_all_tasks();

    char line[LINE_BUF];
    if (!fgets(line, sizeof line, f)) { fclose(f); return 0; } /* header row */

    while (fgets(line, sizeof line, f)) {
        trim_newline(line);
        if (strlen(line) == 0) continue;

        char *fields[5] = {0};
        int fld = csv_parse_line(line, fields);

        if (fld >= 5) {
            Task *t = create_task(fields[1], fields[2], atoi(fields[3]), fields[4]);
            t->id = atoi(fields[0]);
            list_insert_tail(t);
            hash_insert(t);
            heap_insert(t);
            task_bump_next_id(t->id);
        }
        for (int i = 0; i < fld; ++i) free(fields[i]);
    }

    fclose(f);
    printf("Loaded tasks from %s\n", filename);
    return 1;
}
