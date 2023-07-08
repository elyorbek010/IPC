#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>

#include "socket_pipe.h"

#define CHECK_AND_RETURN_IF_NOT_EXIST(object_ptr) \
    do                                            \
    {                                             \
        if (object_ptr == NULL)                   \
        {                                         \
            return EINVAL;                        \
        }                                         \
    } while (0)

struct socket_pipe_t
{
    char socket_file_name[MAX_FILE_NAME_SIZE];
    int sfd; // server specific data
    int data_fd;

    size_t element_size;
};

int socket_pipe_create(size_t element_size, const char *socket_file_name, socket_pipe_t **p_socket_pipe)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(p_socket_pipe);

    int error_code;
    int sfd, data_fd;
    socklen_t peer_addr_size;
    struct sockaddr_un server_addr, client_addr;

    socket_pipe_t *socket_pipe = malloc(sizeof(*socket_pipe));
    if (socket_pipe == NULL)
    {
        free(socket_pipe);
        return errno;
    }

    if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        return errno;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_file_name, sizeof(server_addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
        goto failure;

    if (listen(sfd, 1) == -1)
        goto failure;

    peer_addr_size = sizeof(client_addr);
    if ((data_fd = accept(sfd, (struct sockaddr *)&client_addr, &peer_addr_size)) == -1)
        goto failure;

    memset(socket_pipe, 0, sizeof(socket_pipe));
    socket_pipe->sfd = sfd;
    socket_pipe->data_fd = data_fd;
    socket_pipe->element_size = element_size;
    strncpy(socket_pipe->socket_file_name, socket_file_name, MAX_FILE_NAME_SIZE - 1);

    *p_socket_pipe = socket_pipe;

    if (write(data_fd, &socket_pipe->element_size, sizeof(socket_pipe->element_size)) == -1)
        goto failure;

    return 0;

failure:
    error_code = errno;
    free(socket_pipe);
    close(data_fd);
    close(sfd);
    unlink(socket_file_name);
    return error_code;
}

int socket_pipe_open(const char *socket_file_name, socket_pipe_t **p_socket_pipe)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(p_socket_pipe);

    int error_code;
    int data_fd;
    struct sockaddr_un server_addr;

    socket_pipe_t *socket_pipe = malloc(sizeof(*socket_pipe));
    if (socket_pipe == NULL)
        return errno;

    if ((data_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        free(socket_pipe);
        return errno;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_file_name, sizeof(server_addr.sun_path) - 1);

    while (connect(data_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        if (errno == ENOENT || errno == ECONNREFUSED)
            usleep(100);
        else
            goto failure;
    }

    memset(socket_pipe, 0, sizeof(socket_pipe));
    socket_pipe->data_fd = data_fd;
    strncpy(socket_pipe->socket_file_name, socket_file_name, MAX_FILE_NAME_SIZE - 1);

    if (read(data_fd, &socket_pipe->element_size, sizeof(socket_pipe->element_size)) == -1)
        goto failure;

    *p_socket_pipe = socket_pipe;

    return 0;

failure:
    error_code = errno;
    free(socket_pipe);
    close(data_fd);
    return error_code;
}

int socket_pipe_destroy(socket_pipe_t *socket_pipe)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(socket_pipe);

    int ret = 0;

    ret += close(socket_pipe->data_fd);
    ret += close(socket_pipe->sfd);
    ret += unlink(socket_pipe->socket_file_name);

    free(socket_pipe);

    return ret == 0 ? 0 : errno;
}

int socket_pipe_close(socket_pipe_t *socket_pipe)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(socket_pipe);

    int ret = 0;

    ret = close(socket_pipe->data_fd);

    free(socket_pipe);

    return ret == 0 ? 0 : errno;
}

int socket_pipe_send(socket_pipe_t *socket_pipe, void *element)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(socket_pipe);
    CHECK_AND_RETURN_IF_NOT_EXIST(element);

    return write(socket_pipe->data_fd, element, socket_pipe->element_size) != socket_pipe->element_size ? errno : 0;
}

int socket_pipe_recv(socket_pipe_t *socket_pipe, void *element)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(socket_pipe);
    CHECK_AND_RETURN_IF_NOT_EXIST(element);

    return read(socket_pipe->data_fd, element, socket_pipe->element_size) != socket_pipe->element_size ? errno : 0;
}