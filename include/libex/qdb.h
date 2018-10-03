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

#define QDB_PEEK 1

typedef struct {
    off_t head;
    off_t tail;
    size_t size;
} qdb_t;

int qdb_stat (const char *fname, qdb_t *hdr);

typedef size_t (*serialize_h) (int, void*);
typedef size_t (*deserialize_h) (int, void**);

int qdb_open (const char *fname);
int qdb_put (int fd, void *data, serialize_h fn);
int qdb_get (int fd, void **data, deserialize_h fn);
int qdb_nopen (const char *fname, size_t default_size);
int qdb_nput (int fd, void *data);
int qdb_nget (int fd, void *data, int flags);

#endif // __QDB_H__
