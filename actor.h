#include "cacti.h"
#include "queue.h"

typedef struct actor_s {
    bool is_dead;
    bool is_scheduled;
    void *stateptr;
    queue_t *queue;
    role_t* role;
} actor_t;

typedef struct actor_vector {
    actor_t **data;
    long elements;
    long capacity;
} actor_vector_t;

void free_actor(actor_t*);

actor_vector_t *vec_new() ;

void vec_delete(actor_vector_t *pvec) ;

long vec_add_cell(actor_vector_t *pvec) ;

long vec_length(actor_vector_t *pvec);
