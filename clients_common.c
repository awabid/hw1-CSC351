/*
 * Copyright (c) 2017, Hammurabi Mendes.
 * Licence: BSD 2-clause
 *  
 * 
 * HW1 implementation: Awais Abid 
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdatomic.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "clients_common.h"
#include "networking.h"

atomic_ulong operations_completed;

int flush_buffer(struct client *client);
int obtain_file_size(char *filename);

struct client *make_client(int socket) {
    struct client *new_client = (struct client *) malloc(sizeof(struct client));

    if(new_client != NULL) {
        new_client->socket = socket;
        new_client->state = E_RECV_REQUEST;

        new_client->file = NULL;

        new_client->nread = 0;
        new_client->nwritten = 0;

        new_client->ntowrite = 0;

        new_client->status = STATUS_OK;

        new_client->prev = NULL;
        new_client->next = NULL;
    }

    return new_client;
}

int read_request(struct client *client) {
    ssize_t chunk_size = read(client->socket, client->buffer, BUFFER_SIZE - 1);

    while((chunk_size) > 0) {
        client->nread += chunk_size;

        if(header_complete(client->buffer, client->nread)) {
            char filename[1024];
            char protocol[16];

            if(get_filename(client->buffer, client->nread, filename, 1024, protocol, 16) == 0) {
                switch_state(client, filename, protocol);
                printf("%s", client->buffer);
                return 1;
            }

            //in case get_filename encounters an error
            client->status = STATUS_BAD;
            finish_client(client);
            return 0;
        }

        chunk_size = read(client->socket, client->buffer + client->nread, (BUFFER_SIZE - 1) - client->nread);
    }

    //if encountered an error
    client->status = STATUS_BAD;
    finish_client(client);
    return 0;
}

void switch_state(struct client *client, char *filename, char *protocol) {
    char temporary_buffer[BUFFER_SIZE];

    // Check if the file does not exist.
    //  -If that's the case, we are preparing a 404 "not found" response, and take note of that in client->status
    if(access(filename, F_OK) == -1) {
        get_404(temporary_buffer, filename, protocol);
        client->status = STATUS_404;
    }

    // Check if the file cannot be opened for reading. Check the status of fopen(3) from the standard C library into client->file.
    //  - If that's the case, we are preparing a 403 "forbidden" response, and take note of that in client->status
    else if((client->file = fopen(filename, "r")) == NULL) {
        get_403(temporary_buffer, filename, protocol);
        client->status = STATUS_403;
    }

    // If neither of the above happened, we are preparing a "200 OK" response. The client->status remains STATUS_OK (the default).
    else {
        get_200(temporary_buffer, filename, protocol, obtain_file_size(filename));
    }

    strcpy(client->buffer, temporary_buffer);

    client->ntowrite = strlen(client->buffer);
    client->nwritten = 0;

    client->state = E_SEND_REPLY;
}

int write_reply(struct client *client) {
    // Flush the buffer that contains the header of the response, using flush_buffer
    if(flush_buffer(client) == 0) {
        client->status = STATUS_BAD;
        finish_client(client);
        return 0;
    }

    // If the file is different than NULL, we just sent a "200 OK" response, and now we need to send the file
    if(client->file != NULL) {
        int bytes_read;
        while((bytes_read = fread(client->buffer, sizeof(char), BUFFER_SIZE, client->file)) > 0) {
            client->ntowrite = bytes_read;
            client->nwritten = 0;

            //Flush the buffer that contains the first chunk of the file contents, using flush_buffer
            if(flush_buffer(client) == 0) {
                client->status = STATUS_BAD;
                finish_client(client);
                return 0;
            }

            //Repeat until read is less than BUFFER_SIZE. In this case, reached the end of the file!
            if(bytes_read < BUFFER_SIZE) {
                fclose(client->file);
                client->file = NULL;
                finish_client(client);
                return 1;
            }
        }

        client->status = STATUS_BAD;
        finish_client(client);
        return 0;
    }

    finish_client(client);
    return 1;
}

// Step 2: Complete flush_buffer
/**
 * Flushes the buffer associated with the client \p client. When this function is called,
 * we keep performing writes until client->ntowrite is 0.
 *
 * Every time we write X bytes, we add X to client->nwritten, and subtract X from client->ntowrite.
 *
 * @param client The client we are writing to.
 *
 * @return If at any point the writes return an error, we return 0. Otherwise, we return 1.
 */
int flush_buffer(struct client *client) {
    ssize_t bytes_written = 0;
    if(client->ntowrite < 0 || client->nwritten > 0) {
        return 0;
    }
    while(client->ntowrite > 0) {
        if((bytes_written = write(client->socket, client->buffer, client->ntowrite)) <= 0) {
            return 0;
        }
        client->nwritten += bytes_written;
        client->ntowrite -= bytes_written;
    }

    return 1;
}

// Step 4: Complete obtain_file_size. Use the stat(2) system call.
/**
 * Obtains the file size for the filename passed as parameter.
 *
 * @param filename Filename that will have its size returned.
 *
 * @return Size of the filename provided, or -1 if the size cannot be obtained.
 */
int obtain_file_size(char *filename) {
    // Return the size of the requested file.
    struct stat file_stat;
    if(stat(filename, &file_stat) == 0) {
        return file_stat.st_size;
    }
    return -1;
}

void finish_client(struct client *client) {
    close(client->socket);
    client->socket = -1;
}
