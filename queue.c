#include "queue.h"

#define QUEUE_INITIAL_SIZE 16

void queue_destroy(queue_t *queue) {
    if (queue->array != NULL) {
        free(queue->array);
    }
    free(queue);
}

bool queue_is_empty(queue_t *q) {
    return q->size == 0;
}

unsigned queue_size(queue_t *q) {
    return q->size;
}

void queue_reserve(queue_t *queue, long n) {
    if (queue->array == NULL) {
        queue->array = malloc(sizeof(queue_data_type) * n);
        if (queue->array == NULL) {
            exit(1);
        }
        queue->capacity = n;
    }

    queue_data_type *temp = malloc(sizeof(queue_data_type) * n);
    if (temp == NULL) {
        exit(1);
    }

    long i = 0, j = queue->front;
    while (i < queue->size) {
        temp[i] = queue->array[j];
        j = (j + 1) % queue->capacity;
        i++;
    }

    queue->capacity = n;
    free(queue->array);
    queue->array = temp;
    queue->front = 0;
    queue->back = queue->size - 1;
}

queue_t *queue_create() {
    queue_t *queue = malloc(sizeof(queue_t));
    queue->size = queue->front = queue->back = queue->capacity = 0;
    queue->array = NULL;
    queue_reserve(queue, QUEUE_INITIAL_SIZE);
    return queue;
}

void queue_push(queue_t *queue, queue_data_type item) {
    if (queue->size == queue->capacity) {
        if (queue->capacity == 0) {
            queue_reserve(queue, 1);
        } else {
            queue_reserve(queue, queue->capacity * 2);
        }
    }

    queue->back = (queue->back + 1) % queue->capacity;
    queue->array[queue->back] = item;
    queue->size++;
}

queue_data_type queue_pop(queue_t *queue) {
    queue_data_type item = queue->array[queue->front];
    queue->front = (queue->front + 1) % queue->capacity;
    queue->size--;

    return item;
}
