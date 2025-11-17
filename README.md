#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATAFILE "zentasks_data.txt"

// =============== STRUCTURE DEFINITIONS =================

// Task structure
typedef struct {
    int id;
    char title[100];
    char description[256];
    int priority;
    char due_date[11];
    char status[20];
    int heap_index;  // for priority heap tracking
} Task;

// TaskStore — stores all tasks
typedef struct {
    Task *tasks;
    int count;
    int capacity;
} TaskStore;

// MinHeap for priorities (array of (task_id, priority) pairs)
typedef struct {
    struct {
        int task_id;
        int priority;
    } *heap;
    int size;
    int capacity;
} MinHeap;

// =============== FUNCTION DECLARATIONS =================
void swap_heap_nodes(MinHeap *h, int i, int j, TaskStore *ts);
void heapify_up(MinHeap *h, int i, TaskStore *ts);
void heapify_down(MinHeap *h, int i, TaskStore *ts);
void insert_heap(MinHeap *h, int task_id, int priority, TaskStore *ts);
int extract_min(MinHeap *h, TaskStore *ts);
void update_priority_heap_only(MinHeap *h, int task_id, int new_priority, TaskStore *ts);
int get_int_input(const char *prompt, int min_val, int max_val);
void add_task(TaskStore *ts, MinHeap *h);
void view_tasks(const TaskStore *ts);
void delete_task(TaskStore *ts, MinHeap *h);
void update_task(TaskStore *ts, MinHeap *h);
void save_tasks(const TaskStore *ts);
void load_tasks(TaskStore *ts, MinHeap *h);

// =============== HEAP OPERATIONS =================

void swap_heap_nodes(MinHeap *h, int i, int j, TaskStore *ts) {
    int temp_task_id = h->heap[i].task_id;
    int temp_priority = h->heap[i].priority;
    h->heap[i].task_id = h->heap[j].task_id;
    h->heap[i].priority = h->heap[j].priority;
    h->heap[j].task_id = temp_task_id;
    h->heap[j].priority = temp_priority;

    // update tracked indices in TaskStore if available
    if (ts && h->heap[i].task_id > 0)
        ts->tasks[h->heap[i].task_id - 1].heap_index = i;
    if (ts && h->heap[j].task_id > 0)
        ts->tasks[h->heap[j].task_id - 1].heap_index = j;
}

void heapify_up(MinHeap *h, int i, TaskStore *ts) {
    while (i > 0) {
        int parent = (i - 1) / 2;
        if (h->heap[parent].priority <= h->heap[i].priority)
            break;
        swap_heap_nodes(h, parent, i, ts);
        i = parent;
    }
}

void heapify_down(MinHeap *h, int i, TaskStore *ts) {
    int left, right, smallest;
    while (1) {
        left = 2 * i + 1;
        right = 2 * i + 2;
        smallest = i;

        if (left < h->size && h->heap[left].priority < h->heap[smallest].priority)
            smallest = left;
        if (right < h->size && h->heap[right].priority < h->heap[smallest].priority)
            smallest = right;

        if (smallest != i) {
            swap_heap_nodes(h, i, smallest, ts);
            i = smallest;
        } else
            break;
    }
}

void insert_heap(MinHeap *h, int task_id, int priority, TaskStore *ts) {
    if (h->size >= h->capacity) {
        int newcap = h->capacity * 2;
        if (newcap < 1) newcap = 8;
        void *tmp = realloc(h->heap, sizeof(h->heap[0]) * newcap);
        if (!tmp) {
            perror("realloc heap");
            return;
        }
        h->heap = tmp;
        h->capacity = newcap;
    }

    int i = h->size++;
    h->heap[i].task_id = task_id;
    h->heap[i].priority = priority;
    if (ts && task_id > 0)
        ts->tasks[task_id - 1].heap_index = i;
    heapify_up(h, i, ts);
}

int extract_min(MinHeap *h, TaskStore *ts) {
    if (h->size == 0)
        return -1;

    int root_id = h->heap[0].task_id;
    if (ts && root_id > 0)
        ts->tasks[root_id - 1].heap_index = -1;

    h->heap[0] = h->heap[h->size - 1];
    h->size--;

    if (h->size > 0) {
        if (ts && h->heap[0].task_id > 0)
            ts->tasks[h->heap[0].task_id - 1].heap_index = 0;
        heapify_down(h, 0, ts);
    }

    return root_id;
}

void update_priority_heap_only(MinHeap *h, int task_id, int new_priority, TaskStore *ts) {
    if (!ts || task_id <= 0) return;
    Task *task = &ts->tasks[task_id - 1];
    int index = task->heap_index;
    if (index < 0 || index >= h->size)
        return;
    h->heap[index].priority = new_priority;
    heapify_up(h, index, ts);
    heapify_down(h, index, ts);
}

// =============== USER INPUT =================

int get_int_input(const char *prompt, int min_val, int max_val) {
    int value, success;
    char buffer[256];

    while (1) {
        if (prompt && prompt[0] != '\0') printf("%s", prompt);
        if (!fgets(buffer, sizeof(buffer), stdin)) {
            return -1;
        }

        if (buffer[0] == '\n' || buffer[0] == '\0')
            return 0;

        success = sscanf(buffer, "%d", &value);
        if (success == 1) {
            if (value >= min_val && value <= max_val)
                return value;
            else
                printf("[ERROR] Value must be between %d and %d.\n", min_val, max_val);
        } else {
            printf("[ERROR] Invalid input. Please enter a valid integer.\n");
        }
    }
}

// =============== CORE FUNCTIONS =================

void ensure_taskstore_capacity(TaskStore *ts) {
    if (ts->count < ts->capacity) return;
    int newcap = ts->capacity * 2;
    if (newcap < 4) newcap = 4;
    void *tmp = realloc(ts->tasks, sizeof(Task) * newcap);
    if (!tmp) {
        perror("realloc tasks");
        return;
    }
    ts->tasks = tmp;
    // initialize new slots' heap_index to -1
    for (int i = ts->capacity; i < newcap; ++i) {
        ts->tasks[i].heap_index = -1;
        ts->tasks[i].id = 0;
    }
    ts->capacity = newcap;
}

void add_task(TaskStore *ts, MinHeap *h) {
    ensure_taskstore_capacity(ts);

    Task *t = &ts->tasks[ts->count];
    t->id = ts->count + 1;
    printf("Enter title: ");
    if (!fgets(t->title, sizeof(t->title), stdin)) t->title[0] = '\0';
    t->title[strcspn(t->title, "\n")] = 0;

    printf("Enter description: ");
    if (!fgets(t->description, sizeof(t->description), stdin)) t->description[0] = '\0';
    t->description[strcspn(t->description, "\n")] = 0;

    t->priority = get_int_input("Enter priority (1=High, 5=Low): ", 1, 5);
    printf("Enter due date (YYYY-MM-DD): ");
    if (!fgets(t->due_date, sizeof(t->due_date), stdin)) t->due_date[0] = '\0';
    t->due_date[strcspn(t->due_date, "\n")] = 0;
    strcpy(t->status, "Pending");
    t->heap_index = -1;

    insert_heap(h, t->id, t->priority, ts);
    ts->count++;
    printf("[SUCCESS] Task added successfully (ID %d).\n", t->id);
}

void view_tasks(const TaskStore *ts) {
    if (ts->count == 0) {
        printf("[INFO] No tasks available.\n");
        return;
    }

    printf("\n%-5s %-20s %-10s %-10s %-10s %-10s\n",
           "ID", "Title", "Priority", "Due Date", "Status", "HeapIdx");
    printf("-----------------------------------------------------------------\n");

    for (int i = 0; i < ts->count; i++) {
        const Task *t = &ts->tasks[i];
        printf("%-5d %-20s %-10d %-10s %-10s %-10d\n",
               t->id, t->title, t->priority, t->due_date, t->status, t->heap_index);
    }
}

void delete_task(TaskStore *ts, MinHeap *h) {
    int id = get_int_input("Enter task ID to delete: ", 1, ts->count);
    if (id <= 0 || id > ts->count) {
        printf("[ERROR] Invalid task ID.\n");
        return;
    }
    Task *t = &ts->tasks[id - 1];
    if (t->heap_index >= 0 && t->heap_index < h->size) {
        // Lazy deletion: mark removed in heap by setting to last and heapify
        int idx = t->heap_index;
        h->heap[idx] = h->heap[h->size - 1];
        h->size--;
        if (idx < h->size) { // fix moved node index in TaskStore
            if (h->heap[idx].task_id > 0)
                ts->tasks[h->heap[idx].task_id - 1].heap_index = idx;
            heapify_down(h, idx, ts);
            heapify_up(h, idx, ts);
        }
    }
    t->heap_index = -1;
    strcpy(t->status, "Deleted");
    printf("[SUCCESS] Task %d deleted.\n", id);
}

void update_task(TaskStore *ts, MinHeap *h) {
    int id = get_int_input("Enter task ID to update: ", 1, ts->count);
    if (id <= 0 || id > ts->count) {
        printf("[ERROR] Invalid task ID.\n");
        return;
    }

    Task *task = &ts->tasks[id - 1];
    printf("Updating task #%d - %s\n", id, task->title);
    printf("Enter new priority (1-5): ");
    int new_priority = get_int_input("", 1, 5);
    task->priority = new_priority;
    update_priority_heap_only(h, id, new_priority, ts);
    printf("[SUCCESS] Priority updated.\n");
}

// =============== FILE I/O =================

void save_tasks(const TaskStore *ts) {
    FILE *fp = fopen(DATAFILE, "w");
    if (!fp) {
        perror("fopen save");
        return;
    }
    // Save as CSV: id,priority,title,description,due_date,status
    for (int i = 0; i < ts->count; ++i) {
        const Task *t = &ts->tasks[i];
        // Escape commas are not handled — keep simple CSV where fields do not include commas.
        fprintf(fp, "%d,%d,%s,%s,%s,%s\n",
                t->id, t->priority, t->title, t->description, t->due_date, t->status);
    }
    fclose(fp);
    printf("[INFO] Saved %d tasks to %s\n", ts->count, DATAFILE);
}

void load_tasks(TaskStore *ts, MinHeap *h) {
    FILE *fp = fopen(DATAFILE, "r");
    if (!fp) {
        // No file yet: leave ts empty
        return;
    }

    char line[1024];
    int max_id = 0;
    while (fgets(line, sizeof(line), fp)) {
        // Simple parsing assuming no commas inside fields
        char title[100] = {0}, description[256] = {0}, due[11] = {0}, status[20] = {0};
        int id = 0, prio = 0;
        if (sscanf(line, "%d,%d,%99[^,],%255[^,],%10[^,],%19[^\n]",
                   &id, &prio, title, description, due, status) >= 2) {
            // ensure capacity for this id (we will append in order)
            ensure_taskstore_capacity(ts);
            // Place into next slot (we assume stored in order or we simply append)
            Task *t = &ts->tasks[ts->count];
            t->id = id;
            t->priority = prio;
            strncpy(t->title, title, sizeof(t->title)-1);
            strncpy(t->description, description, sizeof(t->description)-1);
            strncpy(t->due_date, due, sizeof(t->due_date)-1);
            strncpy(t->status, status[0] ? status : "Pending", sizeof(t->status)-1);
            t->heap_index = -1;

            // insert into heap if not deleted
            if (strcmp(t->status, "Deleted") != 0) {
                insert_heap(h, t->id, t->priority, ts);
            }
            ts->count++;
            if (id > max_id) max_id = id;
        }
    }
    fclose(fp);
    // Note: This simple loader appends tasks in file order and leaves ids as recorded.
    // If you want to strictly preserve IDs mapping to array index, you would allocate by max_id and place at id-1.
    printf("[INFO] Loaded %d tasks from %s\n", ts->count, DATAFILE);
}

// =============== MAIN =================

int main() {
    // initial capacities
    int initial_tasks = 10;
    int initial_heap = 16;

    TaskStore ts;
    ts.count = 0;
    ts.capacity = initial_tasks;
    ts.tasks = calloc(ts.capacity, sizeof(Task));
    if (!ts.tasks) {
        perror("calloc tasks");
        return 1;
    }
    // Initialize heap_index sentinel values
    for (int i = 0; i < ts.capacity; ++i) ts.tasks[i].heap_index = -1;

    MinHeap h;
    h.size = 0;
    h.capacity = initial_heap;
    h.heap = malloc(sizeof(h.heap[0]) * h.capacity);
    if (!h.heap) {
        perror("malloc heap");
        free(ts.tasks);
        return 1;
    }

    load_tasks(&ts, &h);

    int choice;
    while (1) {
        printf("\n========== ZEN TASK MANAGER ==========\n");
        printf("1. Add Task\n2. View Tasks\n3. Update Task\n4. Delete Task\n5. Save & Exit\n");
        choice = get_int_input("Enter choice: ", 1, 5);

        if (choice == -1) {
            printf("\n[INFO] EOF received, exiting.\n");
            break;
        }

        switch (choice) {
            case 1: add_task(&ts, &h); break;
            case 2: view_tasks(&ts); break;
            case 3: update_task(&ts, &h); break;
            case 4: delete_task(&ts, &h); break;
            case 5:
                save_tasks(&ts);
                free(ts.tasks);
                free(h.heap);
                printf("Goodbye!\n");
                return 0;
            default:
                printf("[ERROR] Invalid choice.\n");
        }
    }

    save_tasks(&ts);
    free(ts.tasks);
    free(h.heap);
    printf("Goodbye!\n");
    return 0;
}
