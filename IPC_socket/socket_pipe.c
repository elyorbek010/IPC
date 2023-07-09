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
    char socket_file_name[MAX_SOCKET_FILE_NAME_SIZE];
    int sfd; // server specific data
    int data_fd;

    size_t element_size;
};

static int socket_bind(int sfd, const char *socket_file_name)
{
    socklen_t peer_addr_size;
    struct sockaddr_un server_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_file_name, sizeof(server_addr.sun_path) - 1);

    if (bind(sfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) // can't unlink
        return errno;

    return 0;
}

static void socket_bind_clean_up(const char *socket_file_name)
{
    unlink(socket_file_name);
}

static int socket_server_set_up(socket_pipe_t *socket_pipe)
{
    int error_code = 0;
    int sfd = -1;

    // create a socket
    if ((sfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        return errno;

    // name the socket
    if ((error_code = socket_bind(sfd, socket_pipe->socket_file_name)) != 0)
        goto bind_failure;

    // set clients backlog limit
    if (listen(sfd, 1) == -1)
    {
        error_code = errno; // save errno
        goto listen_failure;
    }

    // return some arguments to socket pipe create
    socket_pipe->sfd = sfd;
    return 0;

listen_failure:
    socket_bind_clean_up(socket_pipe->socket_file_name);
bind_failure:
    close(sfd);
    return error_code;
}

static void socket_server_clean_up(socket_pipe_t *socket_pipe)
{
    close(socket_pipe->sfd);
    unlink(socket_pipe->socket_file_name);
}

static int socket_server_connect(socket_pipe_t *socket_pipe)
{
    int error_code = 0;

    int data_fd = -1;
    if ((data_fd = accept(socket_pipe->sfd, NULL, NULL)) == -1) // establish connection with a client
        return errno;

    size_t nbytes = sizeof(socket_pipe->element_size);
    if (write(data_fd, &socket_pipe->element_size, nbytes) != nbytes) // set communication parameters
    {
        error_code = errno;
        close(data_fd);
        return error_code;
    }

    socket_pipe->data_fd = data_fd;
    return 0;
}

int socket_pipe_create(size_t element_size, const char *socket_file_name, socket_pipe_t **p_socket_pipe)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(p_socket_pipe);
    CHECK_AND_RETURN_IF_NOT_EXIST(socket_file_name);

    if (strlen(socket_file_name) >= MAX_SOCKET_FILE_NAME_SIZE)
        return EINVAL;

    int error_code = 0;

    socket_pipe_t *socket_pipe = NULL;
    if ((socket_pipe = malloc(sizeof(*socket_pipe))) == NULL)
        return errno;

    strcpy(socket_pipe->socket_file_name, socket_file_name);
    socket_pipe->element_size = element_size;

    // create socket file
    if ((error_code = socket_server_set_up(socket_pipe)) != 0)
        goto socket_failure;

    // connect to client
    if ((error_code = socket_server_connect(socket_pipe)) != 0)
        goto connect_failure;

    *p_socket_pipe = socket_pipe;

    return 0;

connect_failure:
    socket_server_clean_up(socket_pipe);
socket_failure:
    free(socket_pipe);
    return error_code;
}

int socket_pipe_destroy(socket_pipe_t *socket_pipe)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(socket_pipe);

    int ret = 0;

    ret += close(socket_pipe->sfd);
    ret += close(socket_pipe->data_fd);
    ret += unlink(socket_pipe->socket_file_name);

    free(socket_pipe);

    return ret == 0 ? 0 : errno;
}

static int socket_client_set_up(socket_pipe_t *socket_pipe)
{
    int data_fd;

    if ((data_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
        return errno;

    socket_pipe->data_fd = data_fd;
    return 0;
}

static void socket_client_clean_up(socket_pipe_t *socket_pipe)
{
    close(socket_pipe->data_fd);
}

static int socket_client_connect(socket_pipe_t *socket_pipe)
{
    struct sockaddr_un server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sun_family = AF_UNIX;
    strncpy(server_addr.sun_path, socket_pipe->socket_file_name, sizeof(server_addr.sun_path) - 1);

    uint i = 0;
    while (connect(socket_pipe->data_fd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) == -1 &&
           i < N_TRIES_TO_CONNECT)
    {
        i++;
        if (errno == ENOENT)
            usleep(CONN_RETRY_TIME_US);
        else
            return errno;
    }

    // get communication parameters
    size_t nbytes = sizeof(socket_pipe->element_size);
    if (read(socket_pipe->data_fd, &socket_pipe->element_size, nbytes) != nbytes)
        return errno;

    return 0;
}

int socket_pipe_open(const char *socket_file_name, socket_pipe_t **p_socket_pipe)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(p_socket_pipe);
    CHECK_AND_RETURN_IF_NOT_EXIST(socket_file_name);

    if (strlen(socket_file_name) >= MAX_SOCKET_FILE_NAME_SIZE)
        return EINVAL;

    int error_code;
    int data_fd;

    socket_pipe_t *socket_pipe = NULL;
    if ((socket_pipe = malloc(sizeof(*socket_pipe))) == NULL)
        return errno;

    strcpy(socket_pipe->socket_file_name, socket_file_name);

    if ((error_code = socket_client_set_up(socket_pipe)) != 0)
        goto socket_failure;

    if ((error_code = socket_client_connect(socket_pipe)) != 0)
        goto connect_failure;

    *p_socket_pipe = socket_pipe;

    return 0;

connect_failure:
    socket_client_clean_up(socket_pipe);
socket_failure:
    free(socket_pipe);
    return error_code;
}

int socket_pipe_close(socket_pipe_t *socket_pipe)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(socket_pipe);

    int ret = close(socket_pipe->data_fd);

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