#include <stdio.h>
#include <stdint.h>
#include <time.h>
#include "../include/libex/str.h"
#include "../include/libex/array.h"

DEFINE_ARRAY(int_array_t, int);
DEFINE_SORTED_ARRAY(int_sorted_array_t, int);

void test_array () {
    int_array_t *x;
    INIT_ARRAY(int_array_t, x, 8, 8, NULL);
    printf("%lu %lu %lu %lu\n", x->len, x->bufsize, x->chunk_size, x->data_size);
    for (int i = 0; i < 10; ++i) {
        ARRAY_ADD(x, i);
    }
    for (size_t i = 0; i < x->len; ++i)
        printf("%lu: %d\n", i, x->ptr[i]);
    printf("insert to 5\n");
    ARRAY_INS(x, 50, 5);
    for (size_t i = 0; i < x->len; ++i)
        printf("%lu: %d\n", i, x->ptr[i]);
    printf("delete from 4 of 2\n");
    ARRAY_DEL(x, 4, 2);
    for (size_t i = 0; i < x->len; ++i)
        printf("%lu: %d\n", i, x->ptr[i]);
    printf("delete from 5 of 8\n");
    ARRAY_DEL(x, 5, 8);
    for (size_t i = 0; i < x->len; ++i)
        printf("%lu: %d\n", i, x->ptr[i]);
    printf("%lu %lu %lu %lu\n", x->len, x->bufsize, x->chunk_size, x->data_size);
    ARRAY_FREE(x);
}

int on_compare (int *x, int *y) {
    if (*x > *y) return 1;
    if (*x < *y) return -1;
    return 0;
}

void test_sorted_array () {
    int_sorted_array_t *x;
    INIT_SORTED_ARRAY(int_sorted_array_t, x, 8, 8, NULL, on_compare);
    srand(time(0));
    for (int i = 0; i < 15; ++i) {
        int n = rand();
        size_t idx;
        SORTED_ARRAY_ADD(x, n, idx);
        printf("idx: %lu\n", idx);
    }
    for (size_t i = 0; i < x->len; ++i)
        printf("%lu\t%d\n", i, x->ptr[i]);
    ARRAY_FREE(x);
}

int main () {
    test_array();
    test_sorted_array();
    return 0;
}
