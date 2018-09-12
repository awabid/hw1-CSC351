/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
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

#include "thread_pool.h"

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

	// Step 10: Treat signals the same way that you treated in your server_fork.c (but only SIGPIPE and SIGTERM) [copy/paste!]
	// Treat signals here

	// Start linked list of of clients
	init();

	struct client *current;

	int maximum_descriptor;

	fd_set set_read;
	fd_set set_write;

	while(!done) {
		// Step 11: Zero read and write sets

		// Step 12: Adds the accept socket into the read set

		// Step 13: Iterate over all currently accepted clients [an example of iteration if given below], and:
		//  - If a client's state is E_RECV_REQUEST, add the client to the read set
		//  - If a client's state is E_SEND_REPLY, add the client to the write set
		//
		// While you are doing this, calculate the maximum descriptor number among the acceptance socket,
		// and all the client sockets that you find in the loop above.
		for(current = head; current != NULL; current = current->next) {
			// Code here
		}

		// Step 14: Call select(), blocking until anything can be read from or written to.
		// If a new connection is made, the accept socket is marked as readable;
		// If new data comes from an accepted client, its socket is marked as readable;
		// If new data can be written to an accepted client without blocking, its socket is marked as writeable.

		// result = select(...);

		// Step 15: If you are here, some socket is ready to be written or to be read from.
		// You have two things to do:

		// Step 15.1: Test if the accept socket has been flagged ready for reading. Use the appropriate macro described in select(2).
		//           If so, make the accepted client socket nonblocking, and call insert_client(client_socket);
		if(0/* accept socket flagged for reading */) {
			char host[1024];
			int port;

			int client_socket = accept_client(accept_socket);

			get_peer_information(client_socket, host, 1024, &port);
			printf("New connection from %s, port %d\n", host, port);

			make_nonblocking(client_socket, 1);

			// Inserts the client into the list

			insert_client(client_socket);
		}

		// Step 15.2: Iterate over all currently accepted clients [an example of iteration if given below]
		//           If the client is ready for reading OR ready for writing then call handle_client(client), passing the client pointer.
		for(current = head; current != NULL; current = current->next) {
			// Code here
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
