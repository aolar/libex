#include "hash.h"

hash_key_t hash_str (const char *s, size_t key_len) {
    hash_key_t hash = 0;
    while (*s) {
        hash += *s;
        hash += (hash << 10);
        hash ^= (hash >> 6);
        ++s;
    }
    hash += (hash << 2);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

hash_key_t hash_nstr (const char *s, size_t len) {
    hash_key_t hash = 0;
    for (size_t i = 0; i < len; ++i) {
        hash += s[i];
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 2);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

hash_t *hash_alloc (hash_key_t hash_buf_size, hash_type_t type, calc_h on_hash, compare_h on_compare, copy_h on_copy, free_h on_free) {
    list_t **ptr = calloc(hash_buf_size, sizeof(list_t*));
    if (!ptr)
        return NULL;
    hash_t *hash = calloc(1, sizeof(hash_t));
    if (hash_buf_size < MIN_HASH_SIZE)
        hash_buf_size = MIN_HASH_SIZE;
    hash->size = hash_buf_size;
    hash->max_size = MAX_HASH_SIZE;
    hash->used_size = 0;
    hash->inc_size = 0;
    hash->dec_size = 0;
    hash->ptr = ptr;
    hash->on_hash = on_hash;
    hash->on_compare = on_compare;
    hash->on_copy = on_copy;
    hash->on_free = on_free;
    hash->hist = lst_alloc(NULL);
    hash->type = type;
    return hash;
}

void hash_set_max_size (hash_t *hash, hash_key_t max_size) {
    if (max_size > hash->size)
        hash->max_size = max_size;
}

void hash_free (hash_t *hash) {
    for (hash_key_t i = 0; i < hash->size; ++i) {
        list_t *bucket = hash->ptr[i];
        if (bucket) {
            list_item_t *li = bucket->head;
            if (li && hash->on_free)
                do {
                    hash_item_t *hi = (hash_item_t*)li->ptr;
                    hash->on_free(hi->key, hi->value);
                    free(hi);
                    li = li->next;
                } while (li != bucket->head);
            lfree(bucket);
        }
    }
    lst_free(hash->hist);
    free(hash->ptr);
    free(hash);
}

hash_item_t *hash_get (hash_t *hash, void *key, size_t key_len) {
    hash_key_t idx = hash->on_hash(key, key_len) % hash->size;
    list_t *bucket = hash->ptr[idx];
    if (bucket) {
        list_item_t *li = bucket->head;
        if (li) {
            do {
                hash_item_t *hi = (hash_item_t*)li->ptr;
                if (0 == hash->on_compare(hi->key, key)) {
                    lst_del(hi->h_node);
                    hi->h_node = lst_add(hash->hist, hi);
                    return hi;
                }
                li = li->next;
            } while (li != bucket->head);
        }
    }
    return NULL;
}

double hash_param_filling (hash_t *hash) {
    double size = hash->size, used_size = hash->used_size;
    return used_size / size;
}

double hash_param_conflict (hash_t *hash) {
    double len = hash->len, used_size = hash->used_size;
    return used_size / len;
}

static int on_reuse_item (list_item_t *li, hash_t *hash) {
    hash_item_t *hi = (hash_item_t*)li->ptr;
    list_t *bucket;
    hi->idx = hash->on_hash(hi->key, hi->key_len);
    if (!(bucket = hash->ptr[hi->idx]))
        bucket = hash->ptr[hi->idx] = lst_alloc(NULL);
    hi->b_node = lst_add(bucket, hi);
    return ENUM_CONTINUE;
}

void hash_resize (hash_t *hash, int32_t grow) {
    errno = ERANGE;
    for (hash_key_t i = 0; i < hash->size; ++i) {
        list_t *bucket = hash->ptr[i];
        if (bucket)
            lst_free(bucket);
    }
    hash_key_t newsize = hash->size + grow,
               newbufsize = newsize * sizeof(list_t*);
    list_t **ptr = realloc(hash->ptr, newbufsize);
    if (ptr) {
        hash->ptr = ptr;
        hash->size = newsize;
    } else
        errno = ENOMEM;
    memset(hash->ptr, 0, newbufsize);
    lst_enum(hash->hist, (list_item_h)on_reuse_item, hash, 0);
}

hash_item_t *hash_add (hash_t *hash, void *key, size_t key_len) {
    hash_item_t *hi = NULL;
    hash_key_t idx = hash->on_hash(key, key_len) % hash->size;
    errno = 0;
    list_item_t *li;
    list_t *bucket = hash->ptr[idx];
    if (bucket) {
        li = bucket->head;
        if (li) {
            do {
                hash_item_t *hi = (hash_item_t*)li->ptr;
                if (0 == hash->on_compare(hi->key, key)) {
                    errno = EEXIST;
                    return hi;
                }
                li = li->next;
            } while (li != bucket->head);
        }
    } else {
        hash->ptr[idx] = bucket = lst_alloc(NULL);
        ++hash->used_size;
    }
    if (!(hi = calloc(1, sizeof(hash_item_t))))
        return NULL;
    if (hash->on_copy)
        hi->key = hash->on_copy(key);
    hi->key_len = key_len;
    hi->idx = idx;
    hi->b_node = lst_add(bucket, hi);
    hi->h_node = lst_add(hash->hist, hi);
    hash->len++;
    switch (hash->type) {
        case HASH_VARSIZE:
            if (hash->inc_size && hash->used_size / hash->size > INC_THRESHOLD && hash->max_size > 0 && hash->max_size - hash->inc_size > hash->size)
                hash_resize(hash, hash->inc_size);
            break;
        case HASH_CONSTSIZE:
            if (hash->used_size / hash->size > INC_THRESHOLD && hash->max_size > 0 && hash->max_size - hash->inc_size > hash->size && hash->hist->head && (li = hash->hist->head->prev)) {
                hash_item_t *dhi = (hash_item_t*)li->ptr;
                lst_del(dhi->b_node);
                lst_del(dhi->h_node);
                bucket = hash->ptr[idx];
                if (0 == bucket->len) {
                    lst_free(bucket);
                    hash->ptr[idx] = NULL;
                    --hash->used_size;
                }
                if (hash->on_free)
                    hash->on_free(dhi->key, dhi->value);
                free(dhi);
                errno = ERANGE;
            }
    }
    return hi;
}

void hash_del (hash_t *hash, void *key, size_t key_len) {
    hash_key_t idx = hash->on_hash(key, key_len) % hash->size;
    list_t *bucket = hash->ptr[idx];
    if (bucket) {
        list_item_t *li = bucket->head;
        if (li) {
            do {
                hash_item_t *hi = (hash_item_t*)li->ptr;
                if (0 == hash->on_compare(hi->key, key)) {
                    lst_del(hi->h_node);
                    lst_del(li);
                    --hash->len;
                    if (hash->on_free)
                        hash->on_free(hi->key, hi->value);
                    free(hi);
                    break;
                }
                li = li->next;
            } while (li != bucket->head);
        }
        if (0 == bucket->len) {
            lst_free(bucket);
            hash->ptr[idx] = NULL;
            --hash->used_size;
        }
        switch (hash->type) {
            case HASH_VARSIZE:
                if (hash->dec_size && hash->used_size / hash->size < DEC_THRESHOLD && hash->size - hash->dec_size > MIN_HASH_SIZE)
                    hash_resize(hash, -hash->dec_size);
                break;
            default:
                break;
        }
    }
}

typedef struct {
    hash_item_h on_item;
    list_item_t *item;
    void *userdata;
    int flags;
} hash_enum_t;

static int on_hash_enum (list_item_t *li, hash_enum_t *he) {
    int n = he->on_item((hash_item_t*)li->ptr, he->userdata);
    if (ENUM_BREAK == n && ENUM_STOP_IF_BREAK == he->flags)
        return ENUM_BREAK;
    return ENUM_CONTINUE;
}

void hash_enum (hash_t *hash, hash_item_h on_item, void *userdata, int flags) {
    hash_enum_t he = { .on_item = on_item, .userdata = userdata, .flags = flags };
    lst_enum(hash->hist, (list_item_h)on_hash_enum, (void*)&he, flags);
}
