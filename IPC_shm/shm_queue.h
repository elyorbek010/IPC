#ifndef SHM_QUEUE_H
#define SHM_QUEUE_H

#include <stdlib.h>

#define SHM_PATH "/dev/shm"
#define MAX_FILE_NAME_SIZE 256 // null terminated string, 255 characters
#define MAX_PATH_SIZE 1024

typedef struct shm_queue_t shm_queue_t;

/*
 * Create a queue in shared memory file
 * [in] element size - size of an element in bytes
 * [in] capacity - maximum number of elements in queue, value 0 is invalid
 * [in] shm_file_name - name of the shared memory file to be created, max 255 characters
 * [out] shm_queue - a pointer to the pointer of queue object
 *
 * Returns 0 on success, and errno value on failure
 */
int shm_queue_create(size_t element_size, size_t capacity, const char *shm_file_name, shm_queue_t **p_shm_queue);

/*
 * Map an existing queue in shared memory file to the memory
 * [in] shm_file_name - name of the shared memory file to be opened, max 255 characters
 * [out] shm_queue - a pointer to the pointer of queue object
 *
 * Returns 0 on success, and errno value on failure
 */
int shm_queue_open(const char *shm_file_name, shm_queue_t **p_shm_queue);

/*
 * Close the queue and unmap it from memory
 * Deletes the queue if reference count is 0
 *
 * Returns 0 on success, and errno value on failure
 */
int shm_queue_close(shm_queue_t *shm_queue);

/*
 * Push an element into queue
 * Blocks on overflow
 *
 * Returns 0 on success, and errno on failure
 */
int shm_queue_push(shm_queue_t *shm_queue, const void *element);

/*
 * Pop an element from queue
 * Blocks on underflow
 *
 * Returns 0 on success, and errno on failure
 */
int shm_queue_pop(shm_queue_t *shm_queue, void *element);

#endif // SHM_QUEUE_H