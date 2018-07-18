#include <stdio.h>
#include <stdint.h>
#include "../include/libex/str.h"
#include "../include/libex/array.h"

DEFINE_ARRAY(int_array_t, int);

void test_int_array () {
    int_array_t *x;
    INIT_ARRAY(int, x, 8, 8, NULL);
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

int main () {
    test_int_array();
    return 0;
}
