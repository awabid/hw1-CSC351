/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 */
#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#define NUM_THREADS 16

// Step 16: Implement this interface, according to the specification given in the handout.
//
// put_request is a __producer__ call, which adds a client (struct client *) to a list of requests (struct request *)
//  - Inside your module, you should launch NUM_THREADS __consumer__ threads, which, upon fetching a request, will:
/*
		struct client *client = request->client;

		if(read_request(client)) {
			write_reply(client);
		}

		if(client->status == STATUS_OK) {
			// Atomically increment the operations_completed counter
		}
 */

// start_threads will launch NUM_THREADS, and initialize mutexes and condition varialbes for the producer/consumer.

// finish_threads will set an internal termination flag that will stop the threads, and then all of them will be joined.

struct request {
	struct client *client;

	struct request *next;
};

int start_threads(void);
int finish_threads(void);

void put_request(struct client *client);

#endif /* THREAD_POOL_H */
