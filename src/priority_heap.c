#include <stdio.h>
#include <stdlib.h>
#include "priority_heap.h"

static Task **heap = NULL;
static int heap_size = 0;
static int heap_capacity = 0;

static void heap_ensure_capacity(int cap) {
    if (heap_capacity >= cap) return;
    int newcap = heap_capacity ? heap_capacity * 2 : 16;
    while (newcap < cap) newcap *= 2;
    heap = (Task **)realloc(heap, sizeof(Task *) * newcap);
    if (!heap) { perror("realloc"); exit(1); }
    heap_capacity = newcap;
}

/* Lower priority value wins; ties broken by id so ordering is stable. */
static int heap_cmp(Task *a, Task *b) {
    if (a->priority != b->priority) return a->priority - b->priority;
    return a->id - b->id;
}

static void heap_swap(int i, int j) {
    Task *tmp = heap[i];
    heap[i] = heap[j];
    heap[j] = tmp;
}

static void heap_up(int i) {
    while (i > 1) {
        int parent = i / 2;
        if (heap_cmp(heap[i], heap[parent]) < 0) {
            heap_swap(i, parent);
            i = parent;
        } else break;
    }
}

static void heap_down(int i) {
    while (1) {
        int l = 2 * i, r = 2 * i + 1, smallest = i;
        if (l <= heap_size && heap_cmp(heap[l], heap[smallest]) < 0) smallest = l;
        if (r <= heap_size && heap_cmp(heap[r], heap[smallest]) < 0) smallest = r;
        if (smallest == i) break;
        heap_swap(i, smallest);
        i = smallest;
    }
}

void heap_insert(Task *t) {
    heap_ensure_capacity(heap_size + 2);
    heap_size++;
    heap[heap_size] = t;
    heap_up(heap_size);
}

void heap_remove(Task *t) {
    int idx = -1;
    for (int i = 1; i <= heap_size; ++i) {
        if (heap[i] == t) { idx = i; break; }
    }
    if (idx == -1) return;

    heap[idx] = heap[heap_size];
    heap_size--;
    if (idx <= heap_size) {
        heap_up(idx);
        heap_down(idx);
    }
}

Task *heap_top(void) {
    return heap_size >= 1 ? heap[1] : NULL;
}

void heap_clear(void) {
    heap_size = 0;
}

void heap_destroy(void) {
    free(heap);
    heap = NULL;
    heap_size = heap_capacity = 0;
}
