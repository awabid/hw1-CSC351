/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 * 
 * 
 * 
 * HW1 implementation: Awais Abid
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

//requests list implementation for FIFO request execution
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

//consumer thread function
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
    initialize_request_list();
    atomic_init(&threads_done, false);

    pthread_attr_t config;
    pthread_attr_init(&config);
    pthread_attr_setdetachstate(&config, PTHREAD_CREATE_JOINABLE);

    // Launches all threads -- they are automatically started
    for(int i = 0; i < NUM_THREADS; i++) {
        if(pthread_create(&threadNumber[i], &config, execute_request, NULL) != 0) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

void put_request(struct client *client) {
    struct request *request = (struct request *) malloc(sizeof(struct request *));
    request->client = client;
    request->next = NULL;
    pthread_mutex_lock(&request_list->mutex);

    if(request_list->size == 0) {
        request_list->head = request;
    }
    else {
        request_list->tail->next = request;
    }

    request_list->tail = request;
    request_list->size++;
    pthread_cond_signal(&request_list->empty);
    pthread_mutex_unlock(&request_list->mutex);
}

void *execute_request(void *arg) {
    struct request *request;

    // block if there 's no request
    while(1) {
        pthread_mutex_lock(&request_list->mutex);
        while(request_list->size == 0 && !atomic_load(&threads_done)) {
            pthread_cond_wait(&request_list->empty, &request_list->mutex);
        }

        //if termination flag has been set, stops waiting for requests
        if(atomic_load(&threads_done)) {
            pthread_mutex_unlock(&request_list->mutex);
            break;
        }
        request = request_list->head;
        request_list->head = request_list->head->next;
        request_list->size -= 1;
        pthread_mutex_unlock(&request_list->mutex);

        struct client *client = request->client;
        if(read_request(client)) {
            write_reply(client);
        }

        if(client->status == STATUS_OK) {
            // Increment the number of operations
            atomic_fetch_add(&operations_completed, 1);
        }

        //free () allocated memory to avoid leaks
        free(client);
    }

    pthread_exit(NULL);
}

int finish_threads(void) {
    //update threads_done and broadcast to wakeup all threads for termination
    atomic_store(&threads_done, true);
    pthread_cond_broadcast(&request_list->empty);

    // Blocks main thread until all others return
    for(int i = 0; i < NUM_THREADS; i++) {
        if(pthread_join(threadNumber[i], NULL) != 0) {
            fprintf(stderr, "Error joning threads\n");

            return EXIT_FAILURE;
        }
    }
    pthread_mutex_destroy(&request_list->mutex);
    pthread_cond_destroy(&request_list->empty);
    free(request_list);

    return EXIT_SUCCESS;
}

#endif /* THREAD_POOL_H */
