#include "cacti.h"
#include "actor.h"
#include "threadpool.h"
#include <pthread.h>
#include <string.h>

#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct actor_system_s {
    pthread_mutex_t lock_actor_queues;
    pthread_cond_t notify_all_dead;

    pthread_t sigint_thread;

    long number_of_active_actors;
    threadpool_t *threadpool;
    actor_vector_t *actors;
    bool interrupted;
} actor_system_t;

static pthread_key_t actor_id_key;
static actor_system_t *actor_system;

actor_id_t actor_id_self() {
    return (actor_id_t)pthread_getspecific(actor_id_key);
}

int create_empty_actor(actor_id_t *actor, role_t *role) {
    *actor = vec_add_cell(actor_system->actors);
    actor_system->actors->data[*actor] = malloc(sizeof(actor_t));
    if (actor_system->actors->data[*actor] == NULL) {
        vec_delete(actor_system->actors);
        free(actor_system->threadpool);
        free(actor_system);
        return -1;
    }
    actor_system->number_of_active_actors++;
    actor_t *act = actor_system->actors->data[*actor];
    act->is_dead = act->is_scheduled = false;
    act->queue = queue_create();
    if (act->queue == NULL) {
        free(act);
        return -1;
    }
    act->role = role;
    act->stateptr = NULL;
    return 0;
}

void update_actor_schedule(actor_id_t actor_id, actor_t *actor) {
    if (!actor->is_scheduled && !queue_is_empty(actor->queue)) {
        threadpool_task_t *t = malloc(sizeof(threadpool_task_t));
        if (t == NULL) {
            exit(1);
        }
        t->argument = (void *)actor_id;
        threadpool_schedule(actor_system->threadpool, t);
        actor->is_scheduled = true;
    }
}

int send_message(actor_id_t actor_id, message_t message) {
    pthread_mutex_lock(&(actor_system->lock_actor_queues));

    if (actor_id >= vec_length(actor_system->actors)) {
        pthread_mutex_unlock(&(actor_system->lock_actor_queues));
        return -2;
    }

    actor_t *actor = actor_system->actors->data[actor_id];
    if (actor->is_dead) {
        pthread_mutex_unlock(&(actor_system->lock_actor_queues));
        return -1;
    }
    message_t *m = malloc(sizeof(message_t));
    if (m == NULL) {
        exit(1);
    }
    memcpy(m, &message, sizeof(message_t));
    queue_push(actor->queue, (void *)m);

    update_actor_schedule(actor_id, actor);

    pthread_mutex_unlock(&(actor_system->lock_actor_queues));

    return 0;
}

void threadpool_worker_job(void *arg) {
    threadpool_task_t *t = arg;
    actor_id_t actor_id = (actor_id_t)(t->argument);
    pthread_setspecific(actor_id_key, (void *)actor_id);

    pthread_mutex_lock(&(actor_system->lock_actor_queues));
    actor_t *actor = actor_system->actors->data[actor_id];
    message_t *m = (message_t *)queue_pop(actor->queue);
    actor->is_scheduled = false;
    if (m->message_type == MSG_SPAWN) {
        if (actor_system->interrupted) {
            pthread_mutex_unlock(&(actor_system->lock_actor_queues));
        } else {
            actor_id_t new_id;
            create_empty_actor(&new_id, (role_t *)m->data);
            pthread_mutex_unlock(&(actor_system->lock_actor_queues));
            send_message(new_id, (message_t){MSG_HELLO, sizeof(actor_id_t), (void *)actor_id});
        }
    } else if (m->message_type == MSG_GODIE) {
        actor->is_dead = true;
        pthread_mutex_unlock(&(actor_system->lock_actor_queues));
    } else {
        pthread_mutex_unlock(&(actor_system->lock_actor_queues));
        (actor->role->prompts[m->message_type])(&(actor->stateptr), m->nbytes, m->data);
    }

    free(m);
    free(t);

    if (pthread_mutex_lock(&(actor_system->lock_actor_queues)) != 0) {
        exit(2);
    }
    update_actor_schedule(actor_id, actor);
    if (actor->is_dead && !actor->is_scheduled && queue_is_empty(actor->queue)) {
        actor_system->number_of_active_actors--;
        if (actor_system->number_of_active_actors < 1) {
            if (pthread_cond_signal(&(actor_system->notify_all_dead)) != 0) {
                exit(2);
            }
        }
    }
    if (pthread_mutex_unlock(&(actor_system->lock_actor_queues)) != 0) {
        exit(2);
    }
}

static void *sigint_catcher(void *v) {
    sigset_t signal_set;
    sigemptyset(&signal_set);
    sigaddset(&signal_set, SIGINT);
    sigaddset(&signal_set, SIGTSTP);

    int sig;
    sigwait(&signal_set, &sig);

    if (sig == SIGINT) {
        if (pthread_mutex_lock(&(actor_system->lock_actor_queues)) != 0) {
            exit(2);
        }

        actor_system->interrupted = true;
        for (int i = 0; i < vec_length(actor_system->actors); i++) {
            actor_system->actors->data[i]->is_dead = true;
        }

        if (pthread_mutex_unlock(&(actor_system->lock_actor_queues)) != 0) {
            exit(2);
        }
    }

    return NULL;
}

int actor_system_create(actor_id_t *actor, role_t *const role) {
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, NULL);

    actor_system = malloc(sizeof(actor_system_t));
    if (actor_system == NULL) {
        return -1;
    }
    if ((pthread_mutex_init(&(actor_system->lock_actor_queues), NULL) != 0) ||
        (pthread_cond_init(&(actor_system->notify_all_dead), NULL) != 0)) {
        free(actor_system);
        return -2;
    }

    if (pthread_create(&(actor_system->sigint_thread), NULL, sigint_catcher,
                       (void *)actor_system) != 0) {
        exit(2);
    }

    actor_system->number_of_active_actors = 0;
    actor_system->threadpool = threadpool_create(POOL_SIZE, threadpool_worker_job);
    if (actor_system->threadpool == NULL) {
        free(actor_system);
        return -3;
    }
    actor_system->actors = vec_new();
    if (actor_system->actors == NULL) {
        free(actor_system->threadpool);
        free(actor_system);
        return -4;
    }

    if (create_empty_actor(actor, role) != 0) {
        vec_delete(actor_system->actors);
        threadpool_destroy(actor_system->threadpool);
        free(actor_system);
        return -5;
    }

    actor_system->interrupted = false;
    pthread_key_create(&actor_id_key, NULL);
    send_message(*actor, (message_t){MSG_HELLO, 0, NULL});
    return 0;
}

void actor_system_join(actor_id_t actor) {
    if (actor_system == NULL) {
        exit(1);
    }

    if (pthread_mutex_lock(&(actor_system->lock_actor_queues)) != 0) {
        exit(2);
    }
    if (actor >= vec_length(actor_system->actors)) {
        exit(1);
    }
    while (actor_system->number_of_active_actors > 0) {
        if (pthread_cond_wait(&(actor_system->notify_all_dead),
                              &(actor_system->lock_actor_queues)) != 0) {
            exit(2);
        }
    }
    if (!actor_system->interrupted) {
        pthread_kill(actor_system->sigint_thread, SIGTSTP);
    }
    if (pthread_join(actor_system->sigint_thread, NULL) != 0) {
        exit(1);
    }
    if (pthread_mutex_unlock(&(actor_system->lock_actor_queues)) != 0) {
        exit(2);
    }

    pthread_key_delete(actor_id_key);
    for (int i = 0; i < vec_length(actor_system->actors); i++) {
        free_actor(actor_system->actors->data[i]);
    }

    vec_delete(actor_system->actors);
    threadpool_destroy(actor_system->threadpool);

    pthread_mutex_destroy(&(actor_system->lock_actor_queues));
    pthread_cond_destroy(&(actor_system->notify_all_dead));

    free(actor_system);
}
