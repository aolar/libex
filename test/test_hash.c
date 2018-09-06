#include <time.h>
#include <stdint.h>
#include "../include/libex/hash.h"
#include "../include/libex/str.h"

typedef struct {
    int f1;
    int f2;
} data_t;

// int key
static hash_key_t int_calc (void *key, size_t key_len) {
    return (intptr_t)key;
}

static int int_compare (void *x, void *y) {
    if ((intptr_t)x > (intptr_t)y) return 1;
    if ((intptr_t)x < (intptr_t)y) return -1;
    return 0;
}

static void *int_copy (void *key) {
    return key;
}

static void int_free (void *key, void *value) {
    free(value);
}

// char key
static hash_key_t char_calc (void *key, size_t key_len) {
    return hash_str((const char*)key, 0);
}

static int char_compare (void *x, void *y) {
    return strcmp((const char*)x, (const char*)y);
}

static void *char_copy (void *key) {
    return strdup((const char*)key);
}

static void char_free (void *key, void *value) {
    free(key);
}

void test_hash_1 () {
    hash_t *hash1 = hash_alloc(40000, HASH_VARSIZE, int_calc, int_compare, int_copy, int_free);
    hash_t *hash2 = hash_alloc(40000, HASH_VARSIZE, char_calc, char_compare, char_copy, char_free);
    hash_item_t *h = hash_add(hash1, (void*)5, 0);
    data_t *d = h->value = calloc(1, sizeof(data_t));
    d->f1 = 12;
    d->f2 = 34;
    h = hash_get(hash1, (void*)5, 0);
    d = (data_t*)h->value;
    printf("%d%d\n", d->f1, d->f2);
    h = hash_add(hash2, "test", 0);
    h->value = d;
    h = hash_get(hash2, "test", 0);
    d = (data_t*)h->value;
    printf("%d%d\n", d->f1, d->f2);
    printf("enum: " SIZE_FMT "\n", hash1->len);
    hash_enum(hash1, ({
        int fn (hash_item_t *h, void *dummy) {
            printf("++ %d %d\n", d->f1, d->f2);
            return ENUM_CONTINUE;
        } fn;
    }), NULL, 0);
    printf("hash:\t\n");
    printf("\tsize: " ULONG_FMT  "\n", hash1->size);
    printf("\tused size: " ULONG_FMT  "\n", hash1->used_size);
    printf("\titems: " ULONG_FMT "\n", hash1->len);
    printf("resize\n");
    hash_resize(hash1, 20000);
    printf("enum: " SIZE_FMT "\n", hash1->len);
    hash_enum(hash1, ({
        int fn (hash_item_t *h, void *dummy) {
            printf("-- %d %d\n", d->f1, d->f2);
            return ENUM_CONTINUE;
        } fn;
    }), NULL, 0);
    printf("foreach\n");
    HASH_FOREACH(key, value, hash1)
        printf("- %ld %d\n", (intptr_t)key, ((data_t*)value)->f2);
    HASH_END(hash1);
    h = hash_get(hash1, (void*)5, 0);
    d = (data_t*)h->value;
    printf("get 5: %d%d\n", d->f1, d->f2);
    hash_free(hash2);
    hash_free(hash1);
}

void test_hash_2 () {
    hash_t *h = hash_alloc(2000, HASH_VARSIZE, int_calc, int_compare, int_copy, int_free);
    h->inc_size = 2000;
    for (int i = 0; i < 7000; ++i) {
        hash_key_t k = rand();
        hash_item_t *hi = hash_add(h, (void*)k, 0);
        if (EEXIST != errno) {
            data_t *d = hi->value = calloc(1, sizeof(data_t));
            d->f1 = d->f2 = (hash_key_t)hi->key;
        }
    }
    printf("hash:\t\n");
    printf("\tsize: " ULONG_FMT  "\n", h->size);
    printf("\tused size: " ULONG_FMT  "\n", h->used_size);
    printf("\titems: " ULONG_FMT "\n", h->len);
    printf("\tfilling: %f\n", hash_param_filling(h));
    printf("\tconflict: %f\n", hash_param_conflict(h));
    hash_free(h);
}

int on_intenum (hash_item_t *hi, void *dummy) {
    printf("%ld: %ld\n", (intptr_t)hi->key, (intptr_t)hi->value);
    return ENUM_CONTINUE;
}

void test_hash_3 () {
    hash_t *h = hash_alloc(2000, HASH_VARSIZE, int_calc, int_compare, int_copy, int_free);
    h->inc_size = 2000;
    for (intptr_t i = 0; i < 3000; ++i) {
        hash_item_t *hi = hash_add(h, (void*)i, sizeof(intptr_t));
        if (ERANGE == errno)
            printf("resized. size: %ld\n", h->size);
        hi->value = (void*)i;
    }
    hash_enum(h, on_intenum, NULL, 0);
    hash_free(h);
}

void test_hash_4 () {
    hash_t *h = hash_alloc(2000, HASH_CONSTSIZE, int_calc, int_compare, int_copy, int_free);
    for (intptr_t i = 0; i < 3000; ++i) {
        hash_item_t *hi = hash_add(h, (void*)i, sizeof(intptr_t));
        hi->value = (void*)i;
        if (ERANGE == errno)
            printf("clear. size: %ld\n", h->size);
    }
    //hash_enum(h, on_intenum, NULL, 0);
    HASH_FOREACH(key, value, h)
        printf("%ld: %ld\n", (intptr_t)key, (intptr_t)value);
    HASH_END(h)
    hash_free(h);
}

int main () {
    srand(time(0));
//    test_hash_1();
//    test_hash_2();
//    test_hash_3();
    test_hash_4();
}
