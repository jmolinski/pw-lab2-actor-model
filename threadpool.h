#ifndef _THREADPOOL_H_
#define _THREADPOOL_H_

#include <stdbool.h>
#include "queue.h"
#include "cacti.h"

/**
 *  @struct threadpool_task
 *  @brief the work struct
 *
 *  @var function Pointer to the function that will perform the task.
 *  @var argument Argument to be passed to the function.
 */
typedef struct {
    void *argument;
} threadpool_task_t;


/**
 *  @struct threadpool
 *  @brief The threadpool struct
 *
 * Structural Definition of Thread Pool
 *  @var lock         Mutex Locks for Internal Work
 *  @var notify       Conditional variables for interthread notifications
 *  @var threads      Thread array, represented here by pointer, array name = first element pointer
 *  @var thread_count Number of threads
 *  @var queue        An array of stored tasks, namely task queues
 *  @var shutdown     Indicates whether the thread pool is closed
 *  @var started      Number of threads started
 */
typedef struct threadpool_t {
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    queue_t *task_queue;
    int thread_count;
    bool shutdown;
    void (*worker_job)(void*);
} threadpool_t;

// Create a thread pool.
threadpool_t *threadpool_create(unsigned thread_count, void (*routine)(void *));

// Schedule a new task by passing its argument.
void threadpool_schedule(threadpool_t *pool, threadpool_task_t* arg);

// Destroy thread pool.
void threadpool_destroy(threadpool_t *pool);

#endif /* _THREADPOOL_H_ */
