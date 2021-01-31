#include "cacti.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_RUN_FACTORIAL 1
#define MSG_SAVE_CHILD_ID 2
#define MSG_PROCESS 3
#define MSG_CLEANUP 4

typedef struct factorial_data {
    unsigned target;
    unsigned current_k;
    unsigned long long partial_fac;
} factorial_data_t;

typedef struct actor_state {
    bool is_first;
    actor_id_t child_id;
    actor_id_t parent_id;
    factorial_data_t *fact;
} actor_state_t;

void hello(void **stateptr, size_t nbytes, void *data);
void init_run_factorial(void **stateptr, size_t nbytes, void *data);
void save_child_id(void **stateptr, size_t nbytes, void *data);
void process(void **stateptr, size_t nbytes, void *data);
void cleanup(void **stateptr, size_t nbytes, void *data);

static act_t prompts[] = {hello, init_run_factorial, save_child_id, process, cleanup};
static role_t role = {5, prompts};

void hello(void **stateptr, size_t nbytes, void *data) {
    printf("hello %i\n", actor_id_self());

    actor_state_t *state = malloc(sizeof(actor_state_t));

    if (nbytes == 0) {
        state->is_first = true;
        state->parent_id = 0;
    } else {
        state->is_first = false;
        state->parent_id = (actor_id_t)data;
        send_message(state->parent_id,
                     (message_t){MSG_SAVE_CHILD_ID, sizeof(actor_id_t), (void *)actor_id_self()});
    }
    *stateptr = state;
}

void init_run_factorial(void **stateptr, size_t nbytes, void *data) {
    printf("init factorial %i\n", actor_id_self());

    actor_state_t *state = *stateptr;

    state->fact = malloc(sizeof(factorial_data_t));
    state->fact->partial_fac = 1;
    state->fact->current_k = 0;
    state->fact->target = (unsigned)data;

    send_message(state->child_id, (message_t){MSG_PROCESS, sizeof(factorial_data_t), state->fact});
}

void process(void **stateptr, size_t nbytes, void *data) {
    printf("process %i\n", actor_id_self());

    actor_state_t *state = *stateptr;
    factorial_data_t *fact = (factorial_data_t *)data;

    if (fact->current_k == fact->target) {
        printf("\n\n\n%llu\n\n\n", fact->partial_fac);  // TODO
        send_message(actor_id_self(), (message_t){MSG_CLEANUP, 0, NULL});
    } else {
        fact->current_k++;
        fact->partial_fac = fact->partial_fac * fact->current_k;
        state->fact = fact;
        send_message(actor_id_self(), (message_t){MSG_SPAWN, sizeof(role_t), (void *)(&role)});
    }
}

void save_child_id(void **stateptr, size_t nbytes, void *data) {
    printf("save child id %i\n", actor_id_self());
    actor_state_t *state = *stateptr;
    state->child_id = (actor_id_t)data;

    send_message(state->child_id, (message_t){MSG_PROCESS, sizeof(factorial_data_t), state->fact});
}

void cleanup(void **stateptr, size_t nbytes, void *data) {
    printf("cleanup %i\n", actor_id_self());
    actor_state_t *state = *stateptr;

    if (!state->is_first) {
        send_message(state->parent_id, (message_t){MSG_CLEANUP, 0, NULL});
    } else {
        free(state->fact);
    }
    free(state);

    send_message(actor_id_self(), (message_t){MSG_GODIE, 0, NULL});
}

int main() {
    unsigned n;
    scanf("%u", &n);

    actor_id_t actor;
    actor_system_create(&actor, &role);
    send_message(actor, (message_t){MSG_RUN_FACTORIAL, sizeof(n), (void *)n});

    actor_system_join(actor);

    return 0;
}
