#include "threadpool.h"
#include <pthread.h>

static void *threadpool_worker(void *threadpool) {
    threadpool_t *pool = threadpool;

    while (true) {
        pthread_mutex_lock(&(pool->lock));

        // wait for tasks or for shutdown
        while ((queue_size(pool->task_queue) == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if (pool->shutdown && queue_size(pool->task_queue) == 0) {
            break;
        }

        threadpool_task_t *task = (threadpool_task_t *)queue_pop(pool->task_queue);
        pthread_mutex_unlock(&(pool->lock));

        (*(pool->worker_job))(task);
    }

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return NULL;
}

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

    if ((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
        (pthread_cond_init(&(pool->notify), NULL) != 0)) {
        exit(1); // TODO
    }

    // start workers
    for (unsigned i = 0; i < thread_count; i++) {
        if (pthread_create(&(pool->threads[i]), NULL, threadpool_worker, (void *)pool) != 0) {
            exit(1); // TODO
        }
        pool->thread_count++;
    }

    return pool;
}

void threadpool_schedule(threadpool_t *pool, threadpool_task_t *arg) {
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        exit(1); // TODO
    }

    queue_push(pool->task_queue, (void *)arg);

    if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
        (pthread_mutex_unlock(&pool->lock) != 0)) {
        exit(1); // TODO
    }
}

static void threadpool_free(threadpool_t *pool) {
    pthread_mutex_lock(&(pool->lock));
    pthread_mutex_destroy(&(pool->lock));
    pthread_cond_destroy(&(pool->notify));

    queue_destroy(pool->task_queue);
    free(pool->threads);
    free(pool);
}

void threadpool_destroy(threadpool_t *pool) {
    if (pthread_mutex_lock(&(pool->lock)) != 0) {
        exit(1);
    }

    pool->shutdown = true;

    // wake up all workers
    if ((pthread_cond_broadcast(&(pool->notify)) != 0) ||
        (pthread_mutex_unlock(&(pool->lock)) != 0)) {
        exit(1); // TODO
    }

    // join all workers
    for (int i = 0; i < pool->thread_count; i++) {
        if (pthread_join(pool->threads[i], NULL) != 0) {
            exit(1); // TODO
        }
    }

    threadpool_free(pool);
}
