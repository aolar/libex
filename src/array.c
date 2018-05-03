#include "array.h"

array_t *array_alloc (size_t data_size, size_t chunk_size) {
    array_t *x = calloc(1, sizeof(array_t));
    x->data_size = data_size;
    x->chunk_size = chunk_size;
    x->len = 0;
    x->bufsize = data_size * chunk_size;
    x->data = malloc(x->bufsize);
    return x;
}

void array_free (array_t *x) {
    free(x->data);
    free(x);
}

static int find (array_t *x, void *key, size_t *idx) {
    int found = 0;
    ssize_t l = 0, r = x->len-1, i = *idx;
    while (l <= r) {
        i = (l + r) >> 1;
        found = x->on_compare(key, x->data + i * x->data_size);
        if (found > 0) l = i + 1; else
        if (found < 0) r = i - 1; else
            break;
    }
    *idx = i;
    return found;
}

ssize_t array_find (array_t *x, void *key) {
    size_t idx;
    if (x->on_compare && 0 == find(x, key, &idx))
        return idx;
    return -1;
}

void *array_add (array_t *x, void *key) {
    size_t idx = 0;
    errno = 0;
    if ((1 + x->len) * x->data_size > x->bufsize) {
        size_t nbufsize = x->bufsize + x->data_size * x->chunk_size;
        void *ndata = realloc(x->data, nbufsize);
        if (!ndata) {
            errno = ERANGE;
            return NULL;
        }
        x->bufsize = nbufsize;
        x->data = ndata;
    }
    if (x->on_compare && x->len > 0) {
        int found = find(x, key, &idx);
        if (found > 0) ++idx;
        if (idx < x->len) {
            void *src = x->data + idx * x->data_size,
                 *dst = src + x->data_size,
                 *end = x->data + x->len * x->data_size;
            size_t size = (uintptr_t)end - (uintptr_t)src;
            memmove(dst, src, size);
        }
    } else
        idx = x->len;
    x->len++;
    return x->data + idx * x->data_size;
}
