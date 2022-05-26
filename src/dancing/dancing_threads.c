#include <assert.h>
#include <limits.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "dancing_threads.h"


typedef enum {
    MT_READY,
    MT_SOLUTION,
    MT_WORK_DONE,
    MT_FINISHED
} MessageType;


typedef struct ThreadData {
    int worker_id;
    struct ThreadControl *control;
    pthread_t thread;
    pthread_mutex_t mutex;
    pthread_cond_t work_available;
    Matrix *matrix;
    struct ThreadData *next_ready_thread;
    int finish;
} ThreadData;


typedef struct {
    MessageType type;
    ThreadData *worker_data;
    union {
        struct {
            long int num_solutions;
            long int search_calls;
        };
    };
} Message;


#define QUEUE_SIZE_PER_THREAD 100


typedef struct {
    int queue_size;
    Message *queue;
    int queue_start;
    int num_queued;
    pthread_mutex_t mutex;
    pthread_cond_t message_available;
    pthread_cond_t space_available;
} Queue;


typedef struct ThreadControl {
    int num_threads;
    ThreadData *threads;
    Queue queue;
    int finish_all;
    ThreadData *first_ready_thread;
} ThreadControl;


static pthread_key_t worker_id_key = 0;


#define DEBUG_THREADS 1

#if DEBUG_THREADS
static void thread_printf(char *fmt, ...) {
    static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    
    va_list args;
    va_start(args, fmt);

    char buffer[2048];
    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);
    
    char worker_str[10];
    ThreadData *data = pthread_getspecific(worker_id_key);
    if (data) {
	    sprintf(worker_str, ", W%d", data->worker_id);
    } else {
	    sprintf(worker_str, "%s", "");
    }

    pthread_mutex_lock(&mutex);
    printf("[%p%s] %s", (void *) pthread_self(), worker_str, buffer);
    pthread_mutex_unlock(&mutex);
    
    fflush(stdout);
}
#else
static void thread_printf(char *fmt, ...) { }
#endif


static void init_queue(Queue *queue, int queue_size) {
    queue->queue_size = queue_size;
    queue->queue = calloc(queue_size, sizeof(Message));

    pthread_mutex_init(&queue->mutex, NULL);
    pthread_cond_init(&queue->message_available, NULL);
    pthread_cond_init(&queue->space_available, NULL);
}


static void teardown_queue(Queue *queue) {
    free(queue->queue);

    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->message_available);
    pthread_cond_destroy(&queue->space_available);
}


static void set_timeout(struct timespec *abstime, int seconds) {
  clock_gettime(CLOCK_REALTIME, abstime);
  abstime->tv_sec += seconds;
}


/**
 * Put a message into the queue, blocking for space if necessary.  The message is copied.
 */
static void put_message(Queue *queue, Message *message) {
	pthread_mutex_lock(&queue->mutex);
	
    while (queue->num_queued >= queue->queue_size) {
	    thread_printf("Waiting for space on queue!\n");
        pthread_cond_wait(&queue->space_available, &queue->mutex);
    }

    queue->queue[(queue->queue_start + queue->num_queued) % queue->queue_size] = *message;
	queue->num_queued++;
    thread_printf("Put one message, queue size is now %d\n", queue->num_queued);

    if (queue->num_queued == 1) {
        thread_printf("Signalling that a message is now available\n");
        pthread_cond_signal(&queue->message_available);
    }
	
	pthread_mutex_unlock(&queue->mutex);
}


/**
 * Get a message from the queue.  Returns FALSE if no messages are available.
 * The message is copied and removed from the queue.
 */
static int get_message(Queue *queue, Message *message) {
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->num_queued <= 0) {
        thread_printf("Waiting for message on queue!\n");
        pthread_cond_wait(&queue->message_available, &queue->mutex);
    }

    assert(queue->num_queued > 0);

    *message = queue->queue[queue->queue_start];
    queue->queue_start = (queue->queue_start + 1) % queue->queue_size;
    queue->num_queued--;
    thread_printf("Got one message, queue size is now %d\n", queue->num_queued);

    if (queue->num_queued == queue->queue_size - 1) {
        thread_printf("Signalling that space is now available\n");
        pthread_cond_signal(&queue->space_available);
    }

    pthread_mutex_unlock(&queue->mutex);
    
    return 1;
}


#define MIN2(a,b) ((a) < (b) ? (a) : (b))
#define MIN3(a,b,c) MIN2(MIN2((a),(b)), (c))


static int get_messages(Queue *queue, Message *message, int max, int timeout) {
    pthread_mutex_lock(&queue->mutex);
    
    if (queue->num_queued <= 0) {
        thread_printf("Waiting for message on queue!\n");
        struct timespec abstime;
        set_timeout(&abstime, timeout);
        //pthread_cond_timedwait(&queue->message_available, &queue->mutex, &abstime);
        pthread_cond_wait(&queue->message_available, &queue->mutex);
    }

    if (queue->num_queued <= 0) {
        thread_printf("Timed out waiting for messages\n");
        pthread_mutex_unlock(&queue->mutex);
        return 0;
    }

    /* Copy as many continguous messages as we can into the buffer (we don't try to follow
       wraparound). */
    int num_to_copy = MIN3(queue->num_queued, queue->queue_size - queue->queue_start, max);
    assert(num_to_copy > 0);
    memcpy(message, &queue->queue[queue->queue_start], num_to_copy * sizeof(Message));

    queue->queue_start = (queue->queue_start + num_to_copy) % queue->queue_size;
    queue->num_queued -= num_to_copy;
    thread_printf("Got %d messages, queue size is now %d\n", num_to_copy, queue->num_queued);

    if (queue->num_queued + num_to_copy == queue->queue_size) {
        thread_printf("Signalling that space is now available\n");
        pthread_cond_broadcast(&queue->space_available);
    }
    
    pthread_mutex_unlock(&queue->mutex);
    
    return num_to_copy;
}


/**
 * Found a solution in this thread; put it in a message to send back to the main thread.
 */
static int thread_solution(Matrix *matrix, ThreadData *data) {
    //thread_printf("Solution in worker %d!\n", data->worker_id);
    Message message;
    message.type = MT_SOLUTION;
    message.worker_data = data;
    //TODO set solution
    //put_message(&data->control->queue, &message);
    return data->control->finish_all;
}


static void process_messages(Matrix *matrix, ThreadControl *control) {
    #define NUM_TO_PROCESS 20
    #define MESSAGE_TIMEOUT 1
    Message messages[NUM_TO_PROCESS];
    int num_messages = get_messages(&control->queue, messages, NUM_TO_PROCESS, MESSAGE_TIMEOUT);
    if (num_messages == 0)
        return;

    matrix->num_messages += num_messages;
    thread_printf("Processing %d messages\n", num_messages);

    int i;
    for (i = 0; i < num_messages; i++) {
        Message *message = &messages[i];
        ThreadData *thread_data = message->worker_data;
        switch (message->type) {
            case MT_READY: {
                thread_printf("Received ready message from worker %d\n", thread_data->worker_id);

                thread_data->next_ready_thread = control->first_ready_thread;
                control->first_ready_thread = message->worker_data;
            } break;

            case MT_SOLUTION: {
                thread_printf("Received solution message from worker %d\n", thread_data->worker_id);
            
                //if (!finish_all)
                //    finish_all |= matrix->solution_callback(matrix, matrix->solution_baton);
            } break;

            case MT_WORK_DONE: {
                thread_printf("Received work done message from worker %d (num solutions %d, search calls %d)\n", thread_data->worker_id, message->num_solutions, message->search_calls);
            
                /* Copy statistics into main matrix. */
                matrix->num_solutions += message->num_solutions;
                matrix->search_calls += message->search_calls;
            } break;

            case MT_FINISHED: {
                thread_printf("Received finished message from worker %d\n", thread_data->worker_id);
            } break;
        }
    }
}


static ThreadData *wait_for_ready_thread(Matrix *matrix, ThreadControl *control) {
    thread_printf("Waiting for ready thread\n");
    while (!control->first_ready_thread) {
        process_messages(matrix, control);
    }
    ThreadData *thread_data = control->first_ready_thread;
    control->first_ready_thread = thread_data->next_ready_thread;
    return thread_data;
}


/**
 * Reached depth level; need to hand rest of this tree to a thread.
 */
static int start_thread_search(Matrix *matrix, ThreadControl *control) {
    /* First wait for a ready threadfree. */
    ThreadData *thread_data = wait_for_ready_thread(matrix, control);
    if (!thread_data)
        return 1;
    
    /* Make a clone of the current matrix for the thread to use. */
    thread_printf("Assigning subsearch %d to worker %d\n", matrix->num_subsearches, thread_data->worker_id);
    
    Matrix *submatrix = clone_matrix(matrix);
    thread_printf("Cloned matrix to %p\n", submatrix);
    
    submatrix->solution_callback = (Callback) thread_solution;
    submatrix->solution_baton = thread_data;
    
    pthread_mutex_lock(&thread_data->mutex);
    thread_data->matrix = submatrix;
    pthread_cond_signal(&thread_data->work_available);
    pthread_mutex_unlock(&thread_data->mutex);
    thread_printf("Signaled worker %d\n", thread_data->worker_id);

    matrix->num_subsearches++;
    
    return 0;
}


static void send_ready(ThreadData *data) {
    Message message;
    message.type = MT_READY;
    message.worker_data = data;
    put_message(&data->control->queue, &message);
}


static void send_work_done(ThreadData *data) {
    Message message;
    message.type = MT_WORK_DONE;
    message.worker_data = data;
    message.num_solutions = data->matrix->num_solutions;
    message.search_calls = data->matrix->search_calls;
    put_message(&data->control->queue, &message);
}


static void send_finished(ThreadData *data) {
    Message message;
    message.type = MT_FINISHED;
    message.worker_data = data;
    put_message(&data->control->queue, &message);
}


static void *thread_worker(ThreadData *data) {
    pthread_setspecific(worker_id_key, data);

    do {
        /* Wait for the main thread to give us work (or tell us to exit). */
	    thread_printf("Waiting for work\n");
        pthread_mutex_lock(&data->mutex);
        send_ready(data);
        pthread_cond_wait(&data->work_available, &data->mutex);
        pthread_mutex_unlock(&data->mutex);
	
        if (data->matrix) {
	        thread_printf("Received work\n");
	        search_matrix_internal(data->matrix, 0, INT_MAX);
	        thread_printf("Finished work\n");
	        send_work_done(data);
	        thread_printf("Destroying matrix %p\n", data->matrix);
	        destroy_matrix(data->matrix);
            data->matrix = NULL;
        }
    } while (!data->finish);
    
    thread_printf("Shutting down\n");
    send_finished(data);

    return NULL;
}


int search_with_threads(Matrix *matrix, int depth_cutoff, int num_threads) {
    if (!worker_id_key) {
	    pthread_key_create(&worker_id_key, NULL);
    }

    matrix->num_messages = 0;
    matrix->num_subsearches = 0;
    
    ThreadControl control;
    memset(&control, 0, sizeof(control));

    control.num_threads = num_threads;

    init_queue(&control.queue, num_threads * QUEUE_SIZE_PER_THREAD);

    /* Create some threads. */
    control.threads = calloc(num_threads, sizeof(ThreadData));
    int i;
    for (i = 0; i < control.num_threads; i++) {
        ThreadData *data = &control.threads[i];
        data->worker_id = i+1;
        data->control = &control;
        pthread_mutex_init(&data->mutex, NULL);
        pthread_cond_init(&data->work_available, NULL);
        pthread_create(&data->thread, NULL, (void *(*)(void*))thread_worker, data);
        thread_printf("Created thread %d as %p\n", data->worker_id, (void *) data->thread);
    }
    
    matrix->depth_callback = (Callback) start_thread_search;
    matrix->depth_baton = &control;

    thread_printf("Beginning search\n");
    int result = search_matrix(matrix, depth_cutoff);
    thread_printf("Finished search\n");
    
    /* Process messages, and when each worker is in ready state, shut it down. */
    for (i = 0; i < control.num_threads; i++) {
        ThreadData *thread_data = wait_for_ready_thread(matrix, &control);

        pthread_mutex_lock(&thread_data->mutex);
        thread_data->finish = 1;
        pthread_cond_signal(&thread_data->work_available);
        pthread_mutex_unlock(&thread_data->mutex);
	    thread_printf("Told worker %d to finish\n", thread_data->worker_id);
    }
    
    for (i = 0; i < control.num_threads; i++) {
        ThreadData *thread_data = &control.threads[i];
        pthread_join(thread_data->thread, NULL);
        thread_printf("Joined worker %d\n", thread_data->worker_id);

        pthread_mutex_destroy(&thread_data->mutex);
        pthread_cond_destroy(&thread_data->work_available);
    }

    process_messages(matrix, &control);
    
    /* Clean up. */
    free(control.threads);
    teardown_queue(&control.queue);

    return result;
}
