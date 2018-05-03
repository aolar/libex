#ifndef __LIBEX_ARRAY_H__
#define __LIBEX_ARRAY_H__

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include "list.h"

typedef struct {
    void *data;
    size_t data_size;
    size_t chunk_size;
    size_t len;
    size_t bufsize;
    compare_h on_compare;
} array_t;

array_t *array_alloc (size_t data_size, size_t chunk_size);
ssize_t array_find (array_t *x, void *key);
static inline void *array_get (array_t *x, size_t idx) {
    if (idx >= 0 && idx < x->len)
        return x->data + idx * x->data_size;
    return NULL;
}
void array_free (array_t *x);
void *array_add (array_t *x, void *y);

#endif // __LIBEX_ARRAY_H__
