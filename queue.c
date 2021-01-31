#include "queue.h"

struct queue_item {
    queue_data_type contents;
    struct queue_item *next;
};

queue_t *queue_create() {
    queue_t *queue = malloc(sizeof(queue_t));
    queue->head = queue->tail = NULL;
    queue->size = 0;
    return queue;
}

void queue_destroy(queue_t *queue) {
    free(queue);
}

void queue_push(queue_t *queue, queue_data_type contents) {
    struct queue_item *item = malloc(sizeof(struct queue_item));
    item->contents = contents;
    item->next = NULL;
    if (queue->head == NULL) {
        queue->head = queue->tail = item;
    } else {
        queue->tail = queue->tail->next = item;
    }
    queue->size++;
}

queue_data_type queue_pop(queue_t *queue) {
    queue_data_type popped;
    popped = queue->head->contents;
    struct queue_item *next = queue->head->next;
    free(queue->head);
    queue->head = next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;
    return popped;
}

bool queue_is_empty(queue_t *q) {
    return q->size == 0;
}

unsigned queue_size(queue_t *q) {
    return q->size;
}
