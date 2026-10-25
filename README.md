# ZenTask

A console-based task manager written in C, built around three classic data
structures working together: a **doubly linked list** keeps insertion order,
a **hash table** gives O(1) average lookup by task ID, and a **binary
min-heap** keeps the most urgent task always a peek away. Tasks persist to
disk as CSV between runs.

```
========= ZenTask =========
> add
Title: Finish OS assignment
Description: Submit via portal
Priority (integer, lower = more urgent): 1
Due (text, e.g. 2025-12-31 or 'none'): 2026-10-30
Added task id 1

> top
Top task (id 1, priority 1): Finish OS assignment
```

## Why three structures for one task?

Each one is optimized for a different access pattern the CLI needs:

| Structure         | Used for                              | Complexity        |
|--------------------|----------------------------------------|--------------------|
| Doubly linked list | `list` — tasks in the order you added them | O(1) insert, O(n) scan |
| Hash table (chaining, 1031 buckets) | `show`, `edit`, `delete` — find a task by ID | O(1) average |
| Binary min-heap    | `top` — the single most urgent task    | O(log n) insert/remove, O(1) peek |

A task lives in all three at once: it's a node in the list, chained into a
hash bucket by ID, and a pointer sitting in the heap array — insert/delete
keeps all three in sync, and `edit` re-sifts the heap only when priority
actually changes.

## Features

- Add, list, show, edit, and delete tasks
- Priority queue (`top`) backed by a min-heap, not a linear scan
- O(1) average-case lookup by ID via a hash table
- CSV persistence with proper quote-escaping for commas/quotes in free text
- Auto-loads `zentasks.csv` on startup, prompts to save on exit
- Zero memory leaks (verified with Valgrind — see below)

## Build & run

Requires `gcc` and `make`.

```sh
make        # builds ./zentask
./zentask   # run it
make clean  # remove build artifacts
```


## Dark web dashboard (Windows)

ZenTask also includes a small browser-based dashboard. The HTML, CSS and JavaScript frontend talks to a lightweight HTTP server written in C. The server reuses the same task structure, linked list, hash table, priority heap, and CSV save/load code as the console version. Adding, editing, or deleting a task updates `zentasks.csv` automatically.

### Run it on Windows

1. Install GCC/MinGW and make sure `gcc --version` works in PowerShell.
2. Open the project folder (the one containing `build-web.bat`).
3. Double-click `build-web.bat`, or run `./build-web.bat` from PowerShell.
4. Keep the terminal window open and visit **http://127.0.0.1:8080** in Chrome or Edge.
5. To stop the server, return to the terminal and press **Ctrl+C**.

The first launch imports `data/sample_tasks.csv` into `zentasks.csv` if no saved task file exists. After that, the app uses your saved `zentasks.csv`. Back up that file if your tasks matter to you.

### Web dashboard features

- Add, edit, and delete tasks from the browser
- Search by title, description, date, or task ID
- Filter by due today, overdue, and priority
- View the most urgent task, based on the existing priority heap
- Automatic CSV persistence after each change
- Responsive dark layout for desktop and mobile

The web server is intended for local use on your own computer. It binds to `127.0.0.1` and is not designed to be exposed directly to the public internet.

## Usage

```
add              Add a new task (prompts for title, description, priority, due date)
list             List all tasks in insertion order
show <id>        Show full details for one task
top              Show the highest-priority task
delete <id>      Delete a task by ID
edit <id>        Edit a task's fields (blank = keep current value)
save [file]      Save tasks to CSV (default: zentasks.csv)
load [file]      Load tasks from CSV, replacing what's in memory
help             Show the command list
exit             Prompts to save, then quits
```

Try it with the sample data:

```sh
cp data/sample_tasks.csv zentasks.csv
./zentask
```

## Project layout

```
zentask/
├── include/            # public headers, one per module
│   ├── task.h           struct Task + shared constants
│   ├── linked_list.h     insertion-order list
│   ├── hash_table.h      O(1) lookup by id
│   ├── priority_heap.h   min-heap for "top"
│   ├── file_io.h         CSV save/load
│   └── commands.h        CLI command handlers
├── src/                # implementation, mirrors include/
│   ├── task.c
│   ├── linked_list.c
│   ├── hash_table.c
│   ├── priority_heap.c
│   ├── file_io.c
│   ├── commands.c
│   ├── main.c            CLI loop
│   └── web_server.c      local HTTP server for browser UI
├── web/                  # HTML, CSS and JavaScript dashboard
├── build-web.bat         # build and launch web version on Windows
├── data/
│   └── sample_tasks.csv  example data to load
├── Makefile
└── LICENSE
```

## Testing it was built right

```sh
make
echo "list" | ./zentask                       # smoke test
valgrind --leak-check=full ./zentask           # 0 leaks, 0 errors (verified)
```

## Tech stack

C (C11), linked lists, hash tables (separate chaining), binary heap,
file I/O, Make.

## Possible next steps

- Sort/filter `list` output (by priority, due date)
- Unit tests for each module (CUnit or a small custom harness)
- Replace fixed `HASH_SIZE` with dynamic resizing for very large task counts
