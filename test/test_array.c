#include <stdio.h>
#include <stdint.h>
#include "../include/libex/str.h"
#include "../include/libex/array.h"

int cmp (void *x, void *y) {
    int a = (intptr_t)x;
    int b = *(int*)y;
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

void parray (array_t *x) {
    int *d = (int*)x->data;
    for (size_t i = 0; i < x->len; ++i)
        printf("%d ", d[i]);
    printf("\n");
}

void test_array () {
    void *p;
    array_t *x = array_alloc(sizeof(int), 8);
    printf("array:\n");
    if ((p = array_add(x, NULL)))
        *(int*)p = 3;
    if ((p = array_add(x, NULL)))
        *(int*)p = 7;
    if ((p = array_add(x, NULL)))
        *(int*)p = 4;
    if ((p = array_add(x, NULL)))
        *(int*)p = 10;
    if ((p = array_add(x, NULL)))
        *(int*)p = 2;
    if ((p = array_add(x, NULL)))
        *(int*)p = 6;
    if ((p = array_add(x, NULL)))
        *(int*)p = 9;
    if ((p = array_add(x, NULL)))
        *(int*)p = 10;
    if ((p = array_add(x, NULL)))
        *(int*)p = 19;
    if ((p = array_add(x, NULL)))
        *(int*)p = 110;
    parray(x);
    array_free(x);
}

void test_sorted_array () {
    void *p;
    array_t *x = array_alloc(sizeof(int), 8);
    printf("sorted array:\n");
    x->on_compare = cmp;
    if ((p = array_add(x, (void*)3)))
        *(int*)p = 3;
    if ((p = array_add(x, (void*)7)))
        *(int*)p = 7;
    if ((p = array_add(x, (void*)4)))
        *(int*)p = 4;
    if ((p = array_add(x, (void*)10)))
        *(int*)p = 10;
    if ((p = array_add(x, (void*)2)))
        *(int*)p = 2;
    if ((p = array_add(x, (void*)6)))
        *(int*)p = 6;
    if ((p = array_add(x, (void*)9)))
        *(int*)p = 9;
    if ((p = array_add(x, (void*)10)))
        *(int*)p = 10;
    if ((p = array_add(x, (void*)19)))
        *(int*)p = 19;
    if ((p = array_add(x, (void*)110)))
        *(int*)p = 110;
    parray(x);
    array_free(x);
}

int main () {
    test_array();
    printf("\n");
    test_sorted_array();
    return 0;
}
