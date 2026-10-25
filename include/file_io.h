#ifndef FILE_IO_H
#define FILE_IO_H

/* Writes every task in the linked list to a CSV file (fields are quoted
 * and quote-escaped so titles/descriptions containing commas are safe). */
void save_tasks(const char *filename);

/* Loads tasks from a CSV file previously written by save_tasks(), rebuilding
 * the linked list, hash table and heap. Returns 1 on success, 0 if the file
 * could not be opened. */
int load_tasks(const char *filename);

#endif /* FILE_IO_H */
