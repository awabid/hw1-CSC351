/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause 
 * 
 * 
 * 
 * HW1 implementation: Awais Abid
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/select.h>
#include <signal.h>
#include <errno.h>

#include "server_statemachine.h"

#include "clients_statemachine.h"

#include "networking.h"

//#include "thread_pool.h"

static void setup_signal_handler(int signal, void (*handler)(int));
static void handle_termination(int signal);

static int done = 0;
int server_statemachine(int argc, char **argv) {
    // Syscall results
    int result;

    if(argc < 2) {
        fprintf(stderr, "Usage: server <port>\n");

        return EXIT_FAILURE;
    }

    char *port = argv[1];

    int accept_socket = create_server(atoi(port));

    if(accept_socket == -1) {
        fprintf(stderr, "Error creating server\n");

        return EXIT_FAILURE;
    }

    make_nonblocking(accept_socket, 1);

    // Treat signals 
    setup_signal_handler(SIGTERM, handle_termination);
    setup_signal_handler(SIGPIPE, SIG_IGN);

    // Start linked list of clients
    init();

    struct client *current;

    int maximum_descriptor;

    fd_set set_read;
    fd_set set_write;

    while(!done) {
        // Zero read and write sets
        FD_ZERO(&set_read);
        FD_ZERO(&set_write);

        // Add the accept socket into the read set
        FD_SET(accept_socket, &set_read);
        maximum_descriptor = accept_socket;

        //Iterate over all currently accepted clients, and:
        //  - If a client's state is E_RECV_REQUEST, add the client to the read set
        //  - If a client's state is E_SEND_REPLY, add the client to the write set
        // calculate the maximum descriptor number among the acceptance socket,
        // and all the client sockets in the loop.
        for(current = head; current != NULL; current = current->next) {
            if(current->state == E_RECV_REQUEST) {
                FD_SET(current->socket, &set_read);
            }
            else if(current->state == E_SEND_REPLY) {
                FD_SET(current->socket, &set_write);
            }
            if(current->socket > maximum_descriptor) {
                maximum_descriptor = current->socket;
            }
        }

        // Call select(), blocking until anything can be read from or written to.
        // If a new connection is made, the accept socket is marked as readable;
        // If new data comes from an accepted client, its socket is marked as readable;
        // If new data can be written to an accepted client without blocking, its socket is marked as writeable.
        
        result = select(maximum_descriptor + 1, &set_read, &set_write, NULL, NULL);

        // If you are here, some socket is ready to be written or to be read from.
        // Test if accept socket has been flagged ready for reading and insert client
        if(FD_ISSET(accept_socket, &set_read)) {
            char host[1024];
            int port;

            int client_socket = accept_client(accept_socket);

            get_peer_information(client_socket, host, 1024, &port);
            printf("New connection from %s, port %d\n", host, port);

            make_nonblocking(client_socket, 1);

            //Inserts the client into the list
            insert_client(client_socket);
        }

        // Iterate over all currently accepted clients [an example of iteration if given below]
        //     If the client is ready for reading OR ready for writing then call handle_client(client), passing the client pointer.
        for(current = head; current != NULL; current = current->next) {
            if(FD_ISSET(current->socket, &set_read) || FD_ISSET(current->socket, &set_write)) {
                handle_client(current);
            }
        }

        // Remove dead clients after we process them above
        while(remove_client(-1)) {};
    }

    printf("Finishing program cleanly... %ld operations served\n", operations_completed);

    // If we are here, we got a termination signal
    // Go over all clients and close their sockets
    for(current = head; current != NULL; current = current->next) {
        close(current->socket);
    }

    return EXIT_SUCCESS;
}

void setup_signal_handler(int signal, void (*handler)(int)) {
    struct sigaction request;

    memset(&request, 0, sizeof(struct sigaction));

    request.sa_handler = handler;

    if(sigaction(signal, &request, NULL) == -1) {
        perror("sigaction");

        exit(EXIT_FAILURE);
    }
}

void handle_termination(int signal) {
    done = 1;
}
