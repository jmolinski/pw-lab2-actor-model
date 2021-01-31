/*
jak coś tpool_wait chyba jest jakoś średnio zaimplementowane tutaj, potrafi przestać czekać, jak coś
jeszcze jest w kolejce pracy (u mnie tak było przynajmniej) A wiadomo jak poprawić to tpool_wait? -
ja po prostu patrzę wtedy czy kolejka nie jest pusta w tym ifie w whilu

zamiast zliczac ile watków pracuje, można zliczać ile jeszcze żyje - i na koncu funkcji którą
wykonuje wątek sprawwdzić czy był on ostatni, i jeśli tak - zrobić signal. A w tpool_wait zmienic
dziwny warunek w while na to czy żyjących wątkow jest > 0.
 */

#include "threadpool.h"
#include <pthread.h>

#include <stdio.h>

static void *threadpool_worker(void *threadpool);
static void threadpool_free(threadpool_t *pool);

threadpool_t *threadpool_create(unsigned thread_count, void (*worker_job)(void *)) {
    threadpool_t *pool = (threadpool_t *)malloc(sizeof(struct threadpool_t));
    if (pool == NULL) {
        return NULL;
    }
    pool->thread_count = 0;
    pool->shutdown = false;
    pool->worker_job = worker_job;

    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    if (pool->threads == NULL) {
        free(pool);
        return NULL;
    }
    pool->task_queue = queue_create();
    if (pool->task_queue == NULL) {
        free(pool->threads);
        free(pool);
        return NULL;
    }

    /* init mutex and conditional variable */
    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0)) {
        exit(1);
    }

    /* start worker threads */
    for (unsigned i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
            exit(1); // TODO
        }
        pool->thread_count++;
    }

    return pool;
}

void threadpool_schedule(threadpool_t *pool, threadpool_task_t *arg) {
    /* Mutual exclusion lock ownership must be acquired first */
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        exit(1); // TODO
    }

    /* Add task to queue */
    /* Place function pointers and parameters in tail to add to the task queue */
    queue_push(pool->task_queue, (void *)arg);

    /* pthread_cond_broadcast */
    /*
     * Send out signal to indicate that task has been added
     * If a thread is blocked because the task queue is empty, there will be a wake-up call.
     * If not, do nothing.
     */
    if (pthread_cond_signal(&(pool->notify)) != 0) {
        exit(1); // TODO
    }

    /* Release mutex resources */
    if (pthread_mutex_unlock(&pool->lock) != 0) {
        exit(1); // TODO
    }
}

void threadpool_destroy(threadpool_t *pool) {
    /* Get mutex resources */
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        exit(1);
    }

    /* Gets the specified closing mode */
    pool->shutdown = true;

    /* Wake up all worker threads */
    /* Wake up all threads blocked by dependent variables and release mutexes */
    if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
        (pthread_mutex_unlock(&(pool->lock)) != 0)) {
        exit(1); // TODO
    }

    /* Join all worker thread */
    /* Waiting for all threads to end */
    for (int i = 0; i < pool->thread_count; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            exit(1); // TODO
        }
    }

    threadpool_free(pool);
}

static void threadpool_free(threadpool_t *pool) {
    pthread_mutex_lock(&(pool->lock));
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));

    free(pool->task_queue);
    free(pool->threads);
    free(pool);
}

static void *threadpool_worker(void *threadpool) {
    threadpool_t *pool = threadpool;

    while (true) {
        /* Lock must be taken to wait on conditional variable */
        pthread_mutex_lock(&(pool->lock));

        /* Wait on condition variable, check for spurious wakeups.
           When returning from pthread_cond_wait(), we own the lock. */
        while ((queue_size(pool->task_queue) == 0) && (!pool->shutdown)) {
            /* The task queue is empty and the thread pool is blocked when it is not closed */
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        /* Closing Processing */
        if (pool->shutdown && queue_size(pool->task_queue) == 0) {
            break;
        }

        /* Release mutex */
        pthread_mutex_unlock(&(pool->lock));

        /* Start running tasks */
        (*(pool->worker_job))((threadpool_task_t *)queue_pop(pool->task_queue));
    }

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return NULL;
}
