/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#include "clients_common.h"
#include <stdatomic.h>
#include <pthread.h>
#include <stdbool.h>

#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#define NUM_THREADS 16

struct request {
    struct client *client;

    struct request *next;
};

struct request_list {
    int size;
    struct request *head;
    struct request *tail;
    pthread_mutex_t mutex;
    pthread_cond_t empty;
};

void initialize_request_list();

int start_threads(void);
int finish_threads(void);

void put_request(struct client *client);

void *execute_request(void *);

pthread_t threadNumber[NUM_THREADS];
atomic_bool threads_done;
struct request_list *request_list;

void initialize_request_list() {
    request_list = (struct request_list *) malloc(sizeof(struct request_list));
    request_list->head = NULL;
    request_list->tail = NULL;
    request_list->size = 0;
    pthread_mutex_init(&request_list->mutex, NULL);
    pthread_cond_init(&request_list->empty, NULL);
}

int start_threads(void) {
    //pthread_t threadNumber[NUM_THREADS];
    initialize_request_list();
    atomic_init(&threads_done, false);
    int threadData[NUM_THREADS];

    for(int i = 0; i < NUM_THREADS; i++) {
        threadData[i] = i;
    }

    pthread_attr_t config;
    pthread_attr_init(&config);
    pthread_attr_setdetachstate(&config, PTHREAD_CREATE_JOINABLE);

    // Launches all threads -- they are automatically started
    for(int i = 0; i < NUM_THREADS; i++) {
        if(pthread_create(&threadNumber[i], &config, execute_request, NULL ) != 0) {
            perror("pthread_create");

            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

void put_request(struct client *client) {
    struct request *request = (struct request *) malloc (sizeof(struct request *));
    request->client = client;
    request->next = NULL;
    pthread_mutex_lock(&request_list->mutex);
    if(request_list->size == 0) {
        request_list->head = request;
        //pthread_cond_signal(&request_list->empty);
    }
    else {
        request_list->tail->next = request;
    }
    request_list->tail = request;
    request_list->size++;
    pthread_cond_signal(&request_list->empty);
    pthread_mutex_unlock(&request_list->mutex);
}

void *execute_request(void * arg) {
    struct request *request;
    // getRequest () blocks if there 's no request
    // See the producer - consumer examples
    // in the " Locks & Condition Variables " class
    while(!atomic_load(&threads_done)) {
        pthread_mutex_lock(&request_list->mutex);
        while(request_list->size == 0) {
            pthread_cond_wait(&request_list->empty, &request_list->mutex);
        }
        if(request_list->size == 0) {
            pthread_mutex_unlock(&request_list->mutex);
            continue;
        }
        request = request_list->head;
        request_list->head = request_list->head->next;
        request_list->size-=1;
        pthread_mutex_unlock(&request_list->mutex);

        struct client *client = request->client;
        if(read_request(client)) {
            write_reply(client);
        }
        if(client->status == STATUS_OK) {
            // Increment the number of operations
            atomic_fetch_add(&operations_completed, 1);
        }
        // make sure to free () allocated memory to avoid leaks
        free(client);
    }
    free(request);
    return NULL;
}

int finish_threads(void) {
    // Blocks main thread until all others return
    atomic_store(&threads_done, true);
    pthread_cond_broadcast(&request_list->empty);
    for(int i = 0; i < NUM_THREADS; i++) {
        if(pthread_join(threadNumber[i], NULL) != 0) {
            fprintf(stderr, "Error joning threads\n");

            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}

// Step 16: Implement this interface, according to the specification given in the handout.
//
// put_request is a __producer__ call, which adds a client (struct client *) to a list of requests (struct request *)
//  - Inside your module, you should launch NUM_THREADS __consumer__ threads, which, upon fetching a request, will:
/*


 */

// start_threads will launch NUM_THREADS, and initialize mutexes and condition varialbes for the producer/consumer.

// finish_threads will set an internal termination flag that will stop the threads, and then all of them will be joined.

#endif /* THREAD_POOL_H */
