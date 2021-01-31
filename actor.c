#include "actor.h"

#define INITIAL_SIZE 2

void free_actor(actor_t* act) {
    queue_destroy(act->queue);
    free(act);
}

actor_vector_t *vec_new() {
    actor_vector_t *pvec = (actor_vector_t *)malloc(sizeof(actor_vector_t));
    if (pvec == NULL) {
        return NULL;
    }
    pvec->data = (actor_t **)malloc(sizeof(actor_t*) * INITIAL_SIZE);
    if (pvec->data == NULL) {
        free(pvec);
        return NULL;
    }
    pvec->elements = 0;
    pvec->capacity = INITIAL_SIZE;
    return pvec;
}

void vec_delete(actor_vector_t *pvec) {
    free(pvec->data);
    free(pvec);
}

long vec_add_cell(actor_vector_t *pvec) {
    if(pvec->capacity == pvec->elements) {
        pvec->capacity *= 2;
        pvec->data = realloc(pvec->data, sizeof(actor_t*) * (pvec->capacity));
        if (pvec->data == NULL) {
            exit(1);
        }
    }
    return (pvec->elements)++;
}

long vec_length(actor_vector_t *pvec) {
    return pvec->elements;
}
