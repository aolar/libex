#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "../include/libex/str.h"
#include "../include/libex/tree.h"

static int cmp_int (void *x, void *y) {
    int a = (intptr_t)x, b = (intptr_t)y;
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

static void *copy_int (void *key) {
    return key;
}

static void free_int (void *key, void *value) {
    free(value);
}

static void test_tree1 () {
    rbtree_t *t1 = rbtree_alloc(cmp_int, copy_int, free_int, RBT_UNIQUE);
    tree_item_t *n = rbtree_add(t1, (void*)1);
    n->value = strdup("first");
    n = rbtree_add(t1, (void*)2);
    n->value = strdup("second");
    n = rbtree_add(t1, (void*)3);
    n->value = strdup("third");
    printf("-> enum\n");
    rbtree_enum(t1, NULL, ({
        int fn (tree_item_t *node, void *dummy) {
            printf(SIZE_FMT ": %s\n", (intptr_t)node->key, (char*)node->value);
            return ENUM_CONTINUE;
        } fn;
    }), NULL, 0);
    printf("-> get\n");
    if ((n = rbtree_get(t1, (void*)2)))
        printf("value: %s\n", (char*)n->value);
    rbtree_free(t1);
}

int main () {
    test_tree1();
}
