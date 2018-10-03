#ifndef __LIBEX_TREE_H__
#define __LIBEX_TREE_H__

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "list.h"

#define RBT_UNIQUE 1

typedef struct tree_item tree_item_t;

typedef int (*tree_item_h) (tree_item_t*, void*);

typedef struct rbtree rbtree_t;
struct tree_item {
    void *key;
    void *value;
    struct tree_item *left;
    struct tree_item *right;
    struct tree_item *parent;
    int red;
    rbtree_t *tree;
};

struct rbtree {
    tree_item_t *nil;
    tree_item_t *root;
    size_t len;
    int unique;
    compare_h on_compare;
    copy_h on_copy;
    free_h on_free;
};

rbtree_t *rbtree_alloc (compare_h on_compare, copy_h on_copy, free_h on_free, int unique);
tree_item_t *rbtree_add (rbtree_t *tree, void *key);
tree_item_t *rbtree_get (rbtree_t *tree, void *key);
void rbtree_select (rbtree_t *tree, void *key, tree_item_h on_item, void *userdata, int flags);
void rbtree_del_node (tree_item_t *z);
void rbtree_del_key (rbtree_t *tree, void *key);
void rbtree_enum (rbtree_t *tree, tree_item_t *ns, tree_item_h on_item, void *userdata, int flags);
list_t *rbtree_to_list (rbtree_t *tree);
void rbtree_free (rbtree_t *tree);

#define RBTREE_FOREACH(key, value, tree) { \
    tree_item_t *nil = tree->nil; \
    void enum_node (tree_item_t *x) { \
        if (x->left != nil) enum_node(x->left); \
        void *key = x->key, *value = x->value; \

#define RBTREE_END(tree) \
        if (x->right != nil) enum_node(x->right); \
    } \
    if (tree->root != nil && tree->root->left != nil) \
        enum_node(tree->root->left); \
    }

#endif //  __LIBEX_TREE_H__
