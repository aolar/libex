#include "tree.h"

rbtree_t *rbtree_alloc (compare_h on_compare, copy_h on_copy, free_h on_free, int unique) {
    rbtree_t *new_tree;
    tree_item_t *temp;

    new_tree = (rbtree_t*)malloc(sizeof(rbtree_t));
    new_tree->len = 0;
    new_tree->unique = unique;

    temp = new_tree->nil = (tree_item_t*)malloc(sizeof(tree_item_t));
    temp->parent = temp->left = temp->right = temp;
    temp->red = 0;
    temp->value = temp->key = NULL;
    temp = new_tree->root = (tree_item_t*)malloc(sizeof(tree_item_t));
    temp->parent = temp->left = temp->right = new_tree->nil;
    temp->value = temp->key = NULL;
    temp->red = 0;

    new_tree->on_compare = on_compare;
    new_tree->on_copy = on_copy;
    new_tree->on_free = on_free;
    return new_tree;
}

static void left_rotate (rbtree_t *tree, tree_item_t *x) {
    tree_item_t *y, *nil = tree->nil;

    y = x->right;
    x->right = y->left;
    if (y->left != nil) y->left->parent = x;
    y->parent = x->parent;   
    if( x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

static void right_rotate (rbtree_t *tree, tree_item_t *y) {
    tree_item_t *x, *nil = tree->nil;

    x = y->left;
    y->left = x->right;
    if (nil != x->right)  x->right->parent = y;
    x->parent=y->parent;
    if( y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;
    x->right = y;
    y->parent = x;
}

static tree_item_t *tree_insert_help (rbtree_t *tree, void *key) {
    tree_item_t *x, *y, *nil = tree->nil, *z;
  
    y = tree->root;
    x = tree->root->left;
    while( x != nil) {
        y = x;
        int r = tree->on_compare(x->key, key);
        if (r > 0)
            x = x->left;
        else if (r < 0)
            x = x->right;
        else {
            if (tree->unique) {
                errno = EEXIST;
                return x;
            }
            x = x->right;
        }
    }
    z = calloc(1, sizeof(tree_item_t));
    z->left = z->right=nil;
    z->parent = y;
    z->red = 1;
    z->tree = tree;
    if (tree->on_copy)
        z->key = tree->on_copy(key);
    if ((y == tree->root) || (0 < tree->on_compare(y->key, key)))
        y->left = z;
    else
        y->right = z;
    return z;
}

tree_item_t *rbtree_add (rbtree_t *tree, void *key) {
    tree_item_t *x, *y, *new_node;

    errno = 0;
    x = tree_insert_help(tree, key);
    if (!x || EEXIST == errno)
        return x;
    
    new_node = x;
    x->red = 1;
    while(x->parent->red) {
        if (x->parent == x->parent->parent->left) {
            y=x->parent->parent->right;
            if (y->red) {
                x->parent->red = 0;
                y->red = 0;
                x->parent->parent->red = 1;
                x = x->parent->parent;
            } else {
                if (x == x->parent->right) {
                    x = x->parent;
                    left_rotate(tree, x);
                }
                x->parent->red = 0;
                x->parent->parent->red = 1;
                right_rotate(tree, x->parent->parent);
            } 
        } else {
            y = x->parent->parent->left;
            if (y->red) {
                x->parent->red = 0;
                y->red=0;
                x->parent->parent->red = 1;
                x = x->parent->parent;
            } else {
                if (x == x->parent->left) {
                    x = x->parent;
                    right_rotate(tree,x);
                }
                x->parent->red = 0;
                x->parent->parent->red = 1;
                left_rotate(tree, x->parent->parent);
            } 
        }
    }
    tree->root->left->red = 0;
    ++tree->len;
    return new_node;
}

void rbtree_free (rbtree_t *tree) {
    tree_item_t *nil;
    void fh (tree_item_t *x) {
        if (x != nil) {
            fh(x->left);
            fh(x->right);
            free(x);
        }
    }
    void fhf (tree_item_t *x) {
        if (x != nil) {
            fhf(x->left);
            fhf(x->right);
            tree->on_free(x->key, x->value);
            free(x);
        }
    }
    nil = tree->nil;
    if (tree->on_free)
        fhf(tree->root->left);
    else
        fh(tree->root->left);
    free(tree->root);
    free(tree->nil);
    free(tree);
}

tree_item_t *rbtree_get (rbtree_t *tree, void *key) {
    if (!tree->root || !tree->root->left) return NULL;
    tree_item_t *x = tree->root->left, *nil = tree->nil;
    int n;
    
    if (x == nil) return(0);
    n = tree->on_compare(x->key, key);
    while(0 != n) {
        if (0 < n)
            x = x->left;
        else
            x = x->right;
        if (x == nil) return NULL;
        n = tree->on_compare(x->key, key);
    }
    return x;
}

void rb_delete_fixup (rbtree_t *tree, tree_item_t *x) {
    tree_item_t *root=tree->root->left, *w;

    while((!x->red) && (root != x)) {
        if (x == x->parent->left) {
            w = x->parent->right;
            if (w->red) {
                w->red = 0;
                x->parent->red=1;
                left_rotate(tree, x->parent);
                w = x->parent->right;
            }
            if ((!w->right->red) && (!w->left->red)) { 
                w->red = 1;
                x = x->parent;
            } else {
                if (!w->right->red) {
                    w->left->red = 0;
                    w->red = 1;
                    right_rotate(tree, w);
                    w = x->parent->right;
                }
                w->red = x->parent->red;
                x->parent->red = 0;
                w->right->red = 0;
                left_rotate(tree, x->parent);
                x = root;
            }
        } else {
            w = x->parent->left;
            if (w->red) {
                w->red = 0;
                x->parent->red = 1;
                right_rotate(tree, x->parent);
                w = x->parent->left;
            }
            if ((!w->right->red) && (!w->left->red)) { 
                w->red = 1;
                x = x->parent;
            } else {
                if (!w->left->red) {
                    w->right->red = 0;
                    w->red = 1;
                    left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->red = x->parent->red;
                x->parent->red = 0;
                w->left->red = 0;
                right_rotate(tree, x->parent);
                x = root;
            }
        }
    }
    x->red = 0;
}

static tree_item_t *rbtree_succ (rbtree_t *tree, tree_item_t *x) { 
    tree_item_t* y, *nil=tree->nil, *root=tree->root;

    if (nil != (y = x->right)) {
        while (y->left != nil) y=y->left;
        return(y);
    } else {
        y = x->parent;
        while(x == y->right) {
            x = y;
            y = y->parent;
        }
        if (y == root) return(nil);
        return(y);
    }
}

static tree_item_t *rbtree_pred(rbtree_t *tree, tree_item_t *x) {
    tree_item_t *y, *nil=tree->nil, *root=tree->root;

    if (nil != (y = x->left)) {
        while(y->right != nil)
            y = y->right;
        return(y);
    } else {
        y = x->parent;
        while(x == y->left) { 
            if (y == root) return(nil); 
            x = y;
            y = y->parent;
        }
        return(y);
    }
}

void rbtree_del_node (tree_item_t *z) {
    rbtree_t *tree = z->tree;
    tree_item_t *x, *y, *nil = tree->nil, *root = tree->root;

    errno = 0;
    y = ((z->left == nil) || (z->right == nil)) ? z : rbtree_succ(tree, z);
    x = (y->left == nil) ? y->right : y->left;
    if (root == (x->parent = y->parent))
        root->left=x;
    else {
        if (y == y->parent->left)
            y->parent->left=x;
        else
            y->parent->right=x;
    }
    if (y != z) {
        if (!(y->red)) rb_delete_fixup(tree, x);
        y->left = z->left;
        y->right = z->right;
        y->parent = z->parent;
        y->red = z->red;
        z->left->parent = z->right->parent=y;
        if (z == z->parent->left)
            z->parent->left=y; 
        else
            z->parent->right=y;
        errno = EEXIST;
        --tree->len;
    } else {
        if (!(y->red)) rb_delete_fixup(tree, x);
        if (tree->on_free)
            tree->on_free(y->key, y->value);
        free(y);
        --tree->len;
    }
}

static int on_get_tree_item (tree_item_t *item, list_t *lst) {
    lst_add(lst, item);
    return ENUM_CONTINUE;
}

static int on_del_tree_item (list_item_t *item, void *dummy) {
    rbtree_del_node((tree_item_t*)item->ptr);
    return ENUM_CONTINUE;
}

void rbtree_del_key (rbtree_t *tree, void *key) {
    list_t *lst = lst_alloc(NULL);
    rbtree_select(tree, key, (tree_item_h)on_get_tree_item, (void*)lst, 0);
    lst_enum(lst, (list_item_h)on_del_tree_item, NULL, 0);
    lst_free(lst);
}

void rbtree_enum (rbtree_t *tree, tree_item_t *ns, tree_item_h on_item, void *userdata, int flags) {
    tree_item_t *nil = tree->nil;

    void enum_node (tree_item_t *x) {
        if (x->left != nil)
            enum_node(x->left);
        int n = on_item(x, userdata);
        if (ENUM_BREAK == n && ENUM_STOP_IF_BREAK == flags)
            return;
        if (x->right != nil)
            enum_node(x->right);
    }

    if (NULL == ns && tree->root != nil && tree->root->left != nil) ns = tree->root->left;
    if (ns != NULL && ns != nil) enum_node(ns);
}

static int on_tree_to_list (tree_item_t *item, list_t *lst) {
    lst_adde(lst, item->value);
    return ENUM_CONTINUE;
}

list_t *rbtree_to_list (rbtree_t *tree) {
    list_t *lst = lst_alloc(NULL);
    rbtree_enum(tree, NULL, (tree_item_h)on_tree_to_list, (void*)lst, 0);
    return lst;
}

void rbtree_select (rbtree_t *tree, void *key, tree_item_h on_item, void *userdata, int flags) {
    tree_item_t *x = rbtree_get(tree, key);
    if (x) {
        int rc;
        tree_item_t *n = x;
        while ((n = rbtree_pred(tree, n)) && n != tree->nil && 0 == tree->on_compare(n->key, key))
            if (ENUM_BREAK == (rc = on_item(n, userdata)) && ENUM_STOP_IF_BREAK == flags)
                return;
        if (ENUM_BREAK == on_item(x, userdata) && ENUM_STOP_IF_BREAK == flags)
            return;
        n = x;
        while ((n = rbtree_succ(tree, n)) && n != tree->nil && 0 == tree->on_compare(n->key, key))
            if (ENUM_BREAK == (rc = on_item(n, userdata)) && ENUM_STOP_IF_BREAK == flags)
                return;
    }
}
