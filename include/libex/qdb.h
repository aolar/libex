/**
 * @file qdb.h
 * @brief queue database functions
 */
#ifndef __QDB_H__
#define __QDB_H__

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <sys/file.h>

/** read/write data without changing the current pointer */
#define QDB_PEEK 1

/** @brief A queue dartabase structure */
typedef struct {
    off_t head;         /**< head */
    off_t tail;         /**< tail */
    size_t size;        /**< element size */
} qdb_t;

/** @brief return queue structure
 * @param fname
 * @param hdr
 * @return 0 if success, -1 if error
 */
int qdb_stat (const char *fname, qdb_t *hdr);

/** @brief callback function for write data */
typedef size_t (*serialize_h) (int, void*);
/** @brief callback function for read data */
typedef size_t (*deserialize_h) (int, void**);

/** @brief open queue database
 * @param fname
 * @return file descriptor or -1 if error
 */
int qdb_open (const char *fname);

/** @brief write data to queue
 * @param fd
 * @param data
 * @param fn callback function
 * @return 0 if success, -1 if error
 */
int qdb_put (int fd, void *data, serialize_h fn);

/** @brief read data from queue
 * @param fd
 * @param data
 * @param fn
 * @return 0 if success, -1 if error
 */
int qdb_get (int fd, void **data, deserialize_h fn);

/** @brief open queue database
 * @param fname
 * @param default_size
 * @return file descriptor if success, -1 if error
 */
int qdb_nopen (const char *fname, size_t default_size);

/** @brief write data to queue
 * @param fd
 * @param data
 * @return 0 if success, -1 if error
 */
int qdb_nput (int fd, void *data);

/** @brief read data from queue
 * @param fd
 * @param data
 * @param flags can be 0 of QDB_PEEK
 * @return 0 if success, -1 if error
 */
int qdb_nget (int fd, void *data, int flags);

#endif // __QDB_H__
