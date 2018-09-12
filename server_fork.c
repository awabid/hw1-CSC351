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

#include "server_fork.h"

#include "clients_common.h"

#include "networking.h"

#include "thread_pool.h"

// REMEMBER:
// Making variables and prototypes "static" limits their visibility to the current file!
// If you get an error about "duplicate symbols, either for variables or prototypes, declare them static here

static int done = 0;

int server_fork(int argc, char **argv) {
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

	// Step 7: Call your function to setup signal handlers for SIGPIPE, SIGCHLD, and SIGTERM.

	while(!done) {
		char host[1024];
		int port;

		int client_socket = accept_client(accept_socket);

		// Step 9: Wait for new connections, but ignore interrupted accept_client calls because of SIGCHLD

		get_peer_information(client_socket, host, 1024, &port);
		printf("New connection from %s, port %d\n", host, port);

		struct client *client = make_client(client_socket);

		// Step 8: Fork a different process to handle each client
		//         The children should exit with the status in client->status (defined in clients_common.h)
		if(read_request(client)) {
			write_reply(client);
		}

		if(client->status == STATUS_OK) {
			operations_completed++;
		}

		free(client);
	}

	printf("Finishing program cleanly... %ld operations served\n", operations_completed);

	return EXIT_SUCCESS;
}

// Step 6: Create a function to setup signal handlers, and three handlers:
//  - The handler for SIGPIPE should ignore the signal.
//  - The handler for SIGTERM should set the done flag to 1
//  - The handler for SIGCHLD should collect the exit status of the child, and if it was STATUS_OK (as defined in clients_common.h),
//    you should only then increase the operations_completed variable.
