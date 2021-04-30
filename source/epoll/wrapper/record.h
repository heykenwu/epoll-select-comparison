#ifndef RECORD_H
#define RECORD_H

#define DEFAULT_MAX_SOCKET 1024
#define HOST_NAME_LEN 16

struct Record **init_record_list(size_t size);
void free_record_list(struct Record **records, size_t size);
void add_new_record(struct Record **records, int sd, char *hostname);
void print_and_remove_record(struct Record **records, int sd);
void increment_record(struct Record **records, int sd, int bytes);

#endif