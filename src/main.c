#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "task.h"
#include "hash_table.h"
#include "file_io.h"
#include "commands.h"

/* Parses the trailing "<id>" argument of a command, skipping leading
 * whitespace. Returns -1 if no valid id was given. */
static int parse_id_arg(char *arg) {
    if (!arg) return -1;
    while (*arg && isspace((unsigned char)*arg)) arg++;
    if (!*arg) return -1;
    return atoi(arg);
}

int main(void) {
    hash_clear();

    /* auto-load the default data file if one already exists */
    FILE *f = fopen(DATA_FILE, "r");
    if (f) { fclose(f); load_tasks(DATA_FILE); }

    char line[LINE_BUF];
    print_help();

    while (1) {
        printf("\n> ");
        if (!fgets(line, sizeof line, stdin)) break;
        trim_newline(line);

        char *cmd = line;
        while (*cmd && isspace((unsigned char)*cmd)) cmd++;
        if (!*cmd) continue;

        char *space = strchr(cmd, ' ');
        char *arg = NULL;
        if (space) {
            *space = '\0';
            arg = space + 1;
            while (*arg && isspace((unsigned char)*arg)) arg++;
        }

        if (strcmp(cmd, "add") == 0) {
            cmd_add();
        } else if (strcmp(cmd, "list") == 0) {
            cmd_list();
        } else if (strcmp(cmd, "show") == 0) {
            int id = parse_id_arg(arg);
            if (id <= 0) printf("Usage: show <id>\n");
            else cmd_show(id);
        } else if (strcmp(cmd, "top") == 0) {
            cmd_top();
        } else if (strcmp(cmd, "delete") == 0) {
            int id = parse_id_arg(arg);
            if (id <= 0) printf("Usage: delete <id>\n");
            else cmd_delete(id);
        } else if (strcmp(cmd, "edit") == 0) {
            int id = parse_id_arg(arg);
            if (id <= 0) printf("Usage: edit <id>\n");
            else cmd_edit(id);
        } else if (strcmp(cmd, "save") == 0) {
            const char *fn = (arg && *arg) ? arg : DATA_FILE;
            save_tasks(fn);
        } else if (strcmp(cmd, "load") == 0) {
            const char *fn = (arg && *arg) ? arg : DATA_FILE;
            load_tasks(fn);
        } else if (strcmp(cmd, "help") == 0 || strcmp(cmd, "?") == 0) {
            print_help();
        } else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            printf("Save tasks before exit? (y/N): ");
            if (!fgets(line, sizeof line, stdin)) break;
            if (line[0] == 'y' || line[0] == 'Y') {
                save_tasks(DATA_FILE);
            }
            break;
        } else {
            printf("Unknown command. Type 'help' for commands.\n");
        }
    }

    cleanup_all();
    printf("Bye.\n");
    return 0;
}
