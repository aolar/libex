#ifndef __LIBEX_HASH_H__
#define __LIBEX_HASH_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include "list.h"
#include "str.h"

#define INC_THRESHOLD 0.7
#define DEC_THRESHOLD 0.3
#define MIN_HASH_SIZE 1024
#define MAX_HASH_SIZE INT_MAX

typedef enum { HASH_CONSTSIZE, HASH_VARSIZE } hash_type_t;

#if __x86_64 || __ppc64__
#define MAX_HASH_KEY UINT_MAX
typedef uint64_t hash_key_t;
#else
#define MAX_HASH_KEY ULONG_MAX
typedef uint32_t hash_key_t;
#endif

hash_key_t hash_str (const char *s, size_t len);
hash_key_t hash_nstr (const char *s, size_t len);

typedef struct hash hash_t;
typedef struct {
    void *key;
    void *value;
    size_t key_len;
    hash_key_t idx;
    list_item_t *h_node;
    list_item_t *b_node;
} hash_item_t;

typedef hash_key_t (*calc_h) (void*, size_t);
typedef int (*hash_item_h) (hash_item_t*, void*);

struct hash {
    hash_key_t size;
    hash_key_t max_size;
    hash_key_t used_size;
    int32_t inc_size;
    int32_t dec_size;
    hash_type_t type;
    list_t **ptr;
    list_t *hist;
    hash_key_t len;
    calc_h on_hash;
    compare_h on_compare;
    copy_h on_copy;
    free_h on_free;
};
hash_t *hash_alloc (hash_key_t hash_buf_size, hash_type_t type, calc_h on_hash, compare_h on_compare, copy_h on_copy, free_h on_free);
hash_item_t *hash_get (hash_t *hash, void *key, size_t key_len);
hash_item_t *hash_add (hash_t *hash, void *key, size_t key_len);
void hash_del (hash_t *hash, void *key, size_t key_len);
void hash_enum (hash_t *hash, hash_item_h on_item, void *userdata, int flags);
void hash_resize (hash_t *hash, int32_t grow);
void hash_free (hash_t *hash);

void hash_set_max_size (hash_t *hash, hash_key_t max_size);
double hash_param_filling (hash_t *hash);
double hash_param_conflict (hash_t *hash);

#endif // __LIBEX_HASH_H__
