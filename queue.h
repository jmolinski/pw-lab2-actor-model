#ifndef QUEUE_H
#define QUEUE_H

#include <stdbool.h>
#include <stdlib.h>

typedef void *queue_data_type;

struct queue_root {
    queue_data_type* array;
    long capacity;
    long size;
    long front;
    long back;

    pthread_mutex_t mut;
};

typedef struct queue_root queue_t;

queue_t *queue_create();
void queue_destroy(queue_t *);
void queue_push(queue_t *queue, queue_data_type contents);
queue_data_type queue_pop(queue_t *queue);
bool queue_is_empty(queue_t *q);
unsigned queue_size(queue_t *q);

#endif
