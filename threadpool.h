#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include "cacti.h"
#include "queue.h"
#include <stdbool.h>

typedef struct {
    void *argument;
} threadpool_task_t;

typedef struct threadpool_t {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    queue_t *task_queue;
    int thread_count;
    int alive_threads;
    bool shutdown;
    void (*worker_job)(void *);
} threadpool_t;

// Create a thread pool.
threadpool_t *threadpool_create(unsigned thread_count, void (*routine)(void *));

// Schedule a new task by passing an argument.
void threadpool_schedule(threadpool_t *pool, threadpool_task_t *arg);

// Destroy thread pool.
void threadpool_destroy(threadpool_t *pool);

#endif /* _THREADPOOL_H_ */
