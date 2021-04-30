#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "record.h"

struct Record
{
    char* hostname;
    size_t request_count;
    size_t byte_sent;
};

struct Record **init_record_list(size_t size)
{
    size_t i;
    struct Record **records = (struct Record **)malloc(size * sizeof(struct Record));
    for (i = 0; i < size; i++)
    {
        records[i] = malloc(sizeof(struct Record));
        records[i]->request_count = 0;
        records[i]->byte_sent = 0;
        records[i]->hostname = malloc(HOST_NAME_LEN * sizeof(char));
    }
    return records;
}

void add_new_record(struct Record **records, int sd, char *hostname)
{
    strcpy(records[sd]->hostname, hostname);
}

void print_and_remove_record(struct Record **records, int sd)
{
    printf("IP : %s, request : %ld, sent : %ld byte\n",
           records[sd]->hostname,
           records[sd]->request_count,
           records[sd]->byte_sent);
    records[sd]->request_count = 0;
    records[sd]->byte_sent = 0;
}

void increment_record(struct Record **records, int sd, int bytes)
{
    records[sd]->request_count += 1;
    records[sd]->byte_sent += bytes;
}

void free_record_list(struct Record **records, size_t size)
{
    size_t i;
    for (i = 0; i < size; i++)
    {
        free(records[i]->hostname);
        free(records[i]);
    }
    free(records);
}