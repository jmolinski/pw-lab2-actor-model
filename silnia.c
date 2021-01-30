#include "cacti.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define MSG_RUN_FACTORIAL 1
#define MSG_PING_PARENT 2
#define MSG_SAVE_CHILD_ID 3
#define MSG_PROCESS 4
#define MSG_CLEANUP 5

message_t mk_message(message_type_t type, unsigned size, void *data) {
    message_t m = {type, size, data};
    return m;
}

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

void hello(void **stateptr, size_t nbytes, void *data) {
    actor_state_t *state = malloc(sizeof(actor_state_t));

    if (nbytes == 0) {
        state->is_first = true;
        state->parent_id = 0;
    } else {
        state->is_first = false;
        state->parent_id = (actor_id_t)data;
        send_message(actor_id_self(), mk_message(MSG_PING_PARENT, 0, NULL));
    }

    *stateptr = state;
}

void init_run_factorial(void **stateptr, size_t nbytes, void *data) {
    actor_state_t *state = *stateptr;

    state->fact = malloc(sizeof(factorial_data_t));
    state->fact->partial_fac = 1;
    state->fact->current_k = 0;
    state->fact->target = (unsigned)data;

    send_message(state->child_id, mk_message(MSG_PROCESS, sizeof(factorial_data_t), state->fact));
}

void process(void **stateptr, size_t nbytes, void *data) {
    actor_state_t *state = *stateptr;
    factorial_data_t *fact = (factorial_data_t *)data;

    if (fact->current_k == fact->target) {
        printf("%llu", fact->partial_fac);
        send_message(actor_id_self(), mk_message(MSG_CLEANUP, 0, NULL));
    } else {
        fact->current_k++;
        fact->partial_fac = fact->partial_fac * fact->current_k;
        state->fact = fact;
        send_message(actor_id_self(), mk_message(MSG_SPAWN, 0, NULL));
    }
}

void ping_parent(void **stateptr, size_t nbytes, void *data) {
    actor_state_t *state = *stateptr;
    send_message(state->parent_id,
                 mk_message(MSG_SAVE_CHILD_ID, sizeof(actor_id_t), (void *)actor_id_self()));
}

void save_child_id(void **stateptr, size_t nbytes, void *data) {
    actor_state_t *state = *stateptr;
    state->child_id = (actor_id_t)data;

    send_message(state->child_id, mk_message(MSG_PROCESS, sizeof(factorial_data_t), state->fact));
}

void cleanup(void **stateptr, size_t nbytes, void *data) {
    actor_state_t *state = *stateptr;

    if (!state->is_first) {
        send_message(state->parent_id, mk_message(MSG_CLEANUP, 0, NULL));
    } else {
        free(state->fact);
    }
    free(state);

    send_message(actor_id_self(), mk_message(MSG_GODIE, 0, NULL));
}

int main() {
    unsigned n;
    scanf("%u", &n);

    act_t prompts[] = {hello, init_run_factorial, ping_parent, save_child_id, process, cleanup};
    role_t role = {6, prompts};

    actor_id_t actor;
    actor_system_create(&actor, &role);

    send_message(actor, mk_message(MSG_RUN_FACTORIAL, sizeof(n), (void *)n));

    actor_system_join(actor);

    return 0;
}
