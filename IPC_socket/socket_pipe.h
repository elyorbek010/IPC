#ifndef SOCKET_PIPE_H
#define SOCKET_PIPE_H

#include <stdlib.h>

#define MAX_SOCKET_FILE_NAME_SIZE 108 // null terminated string, limit on linux is 108

#define N_TRIES_TO_CONNECT 10  // number of times client tries to connect
#define CONN_RETRY_TIME_US 100 // time between consecutive tries to connect

typedef struct socket_pipe_t socket_pipe_t;

/*
 * Create a socket pipe
 * [in] element size - size of an element in bytes
 * [in] socket_file_name - name of the socket file to be created, max 255 characters
 * [out] socket_pipe - a pointer to the pointer of pipe object
 *
 * Returns 0 on success, and errno value on failure
 */
int socket_pipe_create(size_t element_size, const char *shm_file_name, socket_pipe_t **p_socket_pipe);

/*
 * Destroy the server socket and clean up
 *
 * Returns 0 on success, and errno value on failure
 */
int socket_pipe_destroy(socket_pipe_t *socket_pipe);

/*
 * Open the socket, connect to the server
 * [in] socket_file_name - name of the socket file to be opened, max 255 characters
 * [out] socket_pipe - a pointer to the pointer of pipe object
 *
 * Returns 0 on success, and errno value on failure
 */
int socket_pipe_open(const char *socket_file_name, socket_pipe_t **p_socket_pipe);

/*
 * Close the socket
 * Returns 0 on success, and errno value on failure
 */
int socket_pipe_close(socket_pipe_t *shm_pipe);

/*
 * Send an element to the client
 *
 * Returns 0 on success, and errno value on failure
 */
int socket_pipe_send(socket_pipe_t *socket_pipe, void *element);

/*
 * Receive an element from server
 *
 * Returns 0 on success, and errno value on failure
 */
int socket_pipe_recv(socket_pipe_t *socket_pipe, void *element);

#endif // SOCKET_PIPE_H