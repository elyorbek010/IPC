#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <string.h>
#include <sys/file.h>
#include <assert.h>
#include <errno.h>

#include "shm_queue.h"

#include <stdio.h>

#define CHECK_AND_RETURN_IF_NOT_EXIST(object_ptr) \
    do                                            \
    {                                             \
        if (object_ptr == NULL)                   \
        {                                         \
            return EINVAL;                        \
        }                                         \
    } while (0)

#define CONCAT2(arg1, arg2) arg1##arg2

#define CONCAT1(arg1, arg2) CONCAT2(arg1, arg2)

#define MACRO_VAR(var_name) CONCAT1(var_name, __LINE__)

#define CODE_BLOCK(start, end) for (int MACRO_VAR(i) = (start, 1); MACRO_VAR(i) != 0; MACRO_VAR(i) = (end, 0))

#if defined(DEBUG) && DEBUG == 1
#define ASSERT(statement) assert(statement)
#else
#define ASSERT(statement) (statement)
#endif

#define CRITICAL_REGION(mutex) CODE_BLOCK(ASSERT(!pthread_mutex_lock(&mutex)), ASSERT(!pthread_mutex_unlock(&mutex)))

struct shm_queue_t
{
    char shm_file_name[MAX_FILE_NAME_SIZE];
    pthread_mutex_t guard;
    pthread_cond_t avail;

    uint ref_cnt;
    int shm_fd;
    size_t shm_size;

    size_t capacity;
    size_t begin;
    size_t end;

    size_t element_size; // in bytes
    char element[];
};

static inline size_t shm_queue_next_index(const size_t index, const size_t element_size, const size_t capacity)
{
    // Note: actual allocated capacity is 'capacity + 1'
    return (index + element_size) % ((capacity + 1) * element_size);
}

static int shm_mutex_init(pthread_mutex_t *mtx)
{
    assert(mtx);

    pthread_mutexattr_t mutex_attr;
    int ret;

    if ((ret = pthread_mutexattr_init(&mutex_attr)))
        return ret;

    if ((ret = pthread_mutexattr_setpshared(&mutex_attr, PTHREAD_PROCESS_SHARED)))
        goto exit;

    if ((ret = pthread_mutex_init(mtx, &mutex_attr)))
        goto exit;

exit:
    pthread_mutexattr_destroy(&mutex_attr);
    return ret;
}

static int shm_cond_init(pthread_cond_t *cond_var)
{
    assert(cond_var);

    pthread_condattr_t condattr;
    int ret;

    if ((ret = pthread_condattr_init(&condattr)))
        return ret;

    if ((ret = pthread_condattr_setpshared(&condattr, PTHREAD_PROCESS_SHARED)))
        goto exit;

    if ((ret = pthread_cond_init(cond_var, &condattr)))
        goto exit;

exit:
    pthread_condattr_destroy(&condattr);
    return ret;
}

static int shm_sync_primitives_init(shm_queue_t *shm_queue)
{
    assert(shm_queue);

    int error_code = 0;

    if (shm_mutex_init(&shm_queue->guard))
        error_code = errno;

    else if (shm_cond_init(&shm_queue->avail))
    {
        error_code = errno;
        pthread_mutex_destroy(&shm_queue->guard);
    }

    return error_code;
}

static void shm_sync_primitives_destroy(shm_queue_t *shm_queue)
{
    pthread_mutex_destroy(&shm_queue->guard);
    pthread_cond_destroy(&shm_queue->avail);
}

static int shm_queue_init(int shm_fd, size_t shm_size, shm_queue_t **p_shm_queue)
{
    int error_code = 0;
    shm_queue_t *shm_queue = NULL;

    if (flock(shm_fd, LOCK_EX)) // Place a lock on file so that no one accesses during initialization
        return errno;

    if (ftruncate(shm_fd, shm_size)) // Enlarge the file to the necessary size
    {
        error_code = errno;
        goto failure;
    }

    // Map the shm_queue object onto shared memory
    if ((shm_queue = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        error_code = errno;
        goto failure;
    }

    if ((error_code = shm_sync_primitives_init(shm_queue))) // init mutex and cond var
        goto sync_init_failure;

    if (flock(shm_fd, LOCK_UN)) // Unlock the file
    {
        error_code = errno;
        goto unlock_failure;
    }

    *p_shm_queue = shm_queue;
    return 0;

unlock_failure:
    shm_sync_primitives_destroy(shm_queue);
sync_init_failure:
    munmap(shm_queue, shm_size);
failure:
    flock(shm_fd, LOCK_UN);
    return error_code;
}

int shm_queue_create(size_t element_size, size_t capacity, const char *shm_file_name, shm_queue_t **p_shm_queue)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(shm_file_name);
    CHECK_AND_RETURN_IF_NOT_EXIST(p_shm_queue);

    if (capacity == 0 || element_size == 0 || strlen(shm_file_name) >= MAX_FILE_NAME_SIZE)
        return EINVAL;

    int error_code = 0;
    int shm_fd = -1;
    shm_queue_t *shm_queue = NULL;

    size_t shm_size = sizeof(*shm_queue) + (capacity + 1) * element_size; // shared memory size: sizeof struct + sizeof elements

    if ((shm_fd = shm_open(shm_file_name, O_CREAT | O_EXCL | O_TRUNC | O_RDWR, 0666)) == -1) // Create shm file
    {
        return errno;
    }

    if ((error_code = shm_queue_init(shm_fd, shm_size, &shm_queue)) != 0)
    {
        close(shm_fd);
        shm_unlink(shm_file_name);
        return error_code;
    }

    strcpy(shm_queue->shm_file_name, shm_file_name);
    shm_queue->ref_cnt = 1;
    shm_queue->shm_fd = shm_fd;
    shm_queue->shm_size = shm_size;
    shm_queue->capacity = capacity;
    shm_queue->begin = 0;
    shm_queue->end = 0;
    shm_queue->element_size = element_size;

    *p_shm_queue = shm_queue;
    return 0;
}

static int shm_file_open(const char *shm_file_name, int *shm_fd)
{
    int fd = -1;

    uint i = 0; // try N_TRIES times if the file does not exist
    while ((fd = shm_open(shm_file_name, O_RDWR, 0666)) == -1 &&
           errno == ENOENT &&
           i++ < N_TRIES)
    {
        usleep(RETRY_TIME_US);
    }

    if (fd == -1)
        return errno;

    *shm_fd = fd;
    return 0;
}

static void shm_file_close(int shm_fd)
{
    close(shm_fd);
}

static ssize_t shm_check_filesize(int fd, char *filepath)
{
    struct stat st;

    if (flock(fd, LOCK_UN))
        return -1;

    if (stat(filepath, &st))
        return -1;

    if (flock(fd, LOCK_UN))
        return -1;

    return st.st_size;
}

static int shm_queue_init_wait(int shm_fd, const char *shm_file_name, size_t *filesize)
{
    char shm_filepath[MAX_PATH_SIZE] = SHM_PATH;
    strncat(shm_filepath, shm_file_name, MAX_PATH_SIZE - strlen(SHM_PATH));

    ssize_t size = 0;
    uint i = 0;
    while ((size = shm_check_filesize(shm_fd, shm_filepath)) == 0 && i++ < N_TRIES)
        usleep(RETRY_TIME_US);

    if (size <= 0)
        return errno;

    *filesize = size;
    return 0;
}

int shm_queue_open(const char *shm_file_name, shm_queue_t **p_shm_queue)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(shm_file_name);
    CHECK_AND_RETURN_IF_NOT_EXIST(p_shm_queue);

    int error_code = 0;
    int shm_fd = -1;
    size_t size = 0;
    shm_queue_t *shm_queue = NULL;

    if ((error_code = shm_file_open(shm_file_name, &shm_fd)) != 0)
        return error_code;

    // Poll until shared file is not initialized yet(size is 0)
    if ((error_code = shm_queue_init_wait(shm_fd, shm_file_name, &size)) != 0)
        goto failure;

    if ((shm_queue = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0)) == MAP_FAILED)
    {
        error_code = errno;
        goto failure;
    }

    // update reference count
    CRITICAL_REGION(shm_queue->guard)
    {
        shm_queue->ref_cnt++;
    }

    *p_shm_queue = shm_queue;
    return 0;

failure:
    shm_file_close(shm_fd);
    return error_code;
}

int shm_queue_close(shm_queue_t *shm_queue)
{
    int ref_cnt = 0;

    if (flock(shm_queue->shm_fd, LOCK_EX)) // lock file so that no new processes open queue when deleting queue
        return errno;                      // flock gets unlocked when file descriptor is closed

    CRITICAL_REGION(shm_queue->guard)
    {
        ref_cnt = --shm_queue->ref_cnt;
    }

    if (ref_cnt == 0)
    {
        shm_sync_primitives_destroy(shm_queue);

        close(shm_queue->shm_fd);
        shm_unlink(shm_queue->shm_file_name);
    }
    else
    {
        close(shm_queue->shm_fd);
    }

    munmap(shm_queue, sizeof(*shm_queue));

    return 0;
}

int shm_queue_push(shm_queue_t *shm_queue, const void *element)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(shm_queue);
    CHECK_AND_RETURN_IF_NOT_EXIST(element);

    CRITICAL_REGION(shm_queue->guard)
    {

        // Block if queue is full
        while (shm_queue_next_index(shm_queue->end, shm_queue->element_size, shm_queue->capacity) == shm_queue->begin)
        {
            pthread_cond_wait(&shm_queue->avail, &shm_queue->guard);
        }

        memcpy(&shm_queue->element[shm_queue->end], element, shm_queue->element_size);
        shm_queue->end = shm_queue_next_index(shm_queue->end, shm_queue->element_size, shm_queue->capacity);
    }

    pthread_cond_signal(&shm_queue->avail);

    return 0;
}

int shm_queue_pop(shm_queue_t *shm_queue, void *element)
{
    CHECK_AND_RETURN_IF_NOT_EXIST(shm_queue);
    CHECK_AND_RETURN_IF_NOT_EXIST(element);

    CRITICAL_REGION(shm_queue->guard)
    {

        // Block if queue is empty
        while (shm_queue->begin == shm_queue->end)
        {
            pthread_cond_wait(&shm_queue->avail, &shm_queue->guard);
        }

        memcpy(element, &shm_queue->element[shm_queue->begin], shm_queue->element_size);
        shm_queue->begin = shm_queue_next_index(shm_queue->begin, shm_queue->element_size, shm_queue->capacity);
    }

    pthread_cond_signal(&shm_queue->avail);

    return 0;
}