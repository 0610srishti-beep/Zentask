#ifndef COMMANDS_H
#define COMMANDS_H

/* Each cmd_* function implements one CLI command, prompting for any input
 * it needs and keeping the linked list / hash table / heap in sync. */

void cmd_add(void);
void cmd_list(void);
void cmd_show(int id);
void cmd_top(void);
void cmd_delete(int id);
void cmd_edit(int id);

void print_help(void);

/* Frees every task and the heap's backing array. */
void cleanup_all(void);

#endif /* COMMANDS_H */
