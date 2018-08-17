/**
 * @file tree.h
 * @brief tree functions
 */
#ifndef __LIBEX_TREE_H__
#define __LIBEX_TREE_H__

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "list.h"

/** tree key is unique */
#define RBT_UNIQUE 1

/** opaque tree item structure */
typedef struct tree_item tree_item_t;

/** callback function for operations on tree items */
typedef int (*tree_item_h) (tree_item_t*, void*);

/** opaque tree structure */
typedef struct rbtree rbtree_t;
/** @brief tree item structure */
struct tree_item {
    void *key;                  /**< key */
    void *value;                /**< value */
    struct tree_item *left;     /**< left branch */
    struct tree_item *right;    /**< right branch */
    struct tree_item *parent;   /**< parent */
    int red;                    /**< is red item */
    rbtree_t *tree;             /**< item owner */
};

/** @brief tree structure */
struct rbtree {
    tree_item_t *nil;           /**< null item of tree */
    tree_item_t *root;          /**< root item of tree */
    size_t len;                 /**< item count */
    int unique;                 /**< is tree contains unique keys */
    compare_h on_compare;       /**< compare callback function */
    copy_h on_copy;             /**< key copy callback function */
    free_h on_free;             /**< free item key and value callback function */
};

/** @brief create tree structure 
 * @param on_compare compare callback function
 * @param on_copy copy key callback function
 * @param on_free free item callback function
 * @param unique is unique key, if 0 then not unique, if RBT_UNIQUE then unique keys
 * @return tree structure
 */
rbtree_t *rbtree_alloc (compare_h on_compare, copy_h on_copy, free_h on_free, int unique);

/** @brief add key into tree, if tree can to contain only unique keys and key is exists then
 * function returns existing item and set errno to EEXIST
 * @param tree
 * @param key
 * @return tree item structure
 */
tree_item_t *rbtree_add (rbtree_t *tree, void *key);

/** @brief returns tree item for \b key, if tree can contains not unique keys 
 * preferably to use \b rbtree_select function
 * @param tree
 * @param key
 * @return tree item
 */
tree_item_t *rbtree_get (rbtree_t *tree, void *key);

/** @brief returns set of items with key
 * @param tree
 * @param key
 * @param on_item callback function executed for each found items
 * @param userdata
 * @param flags
 * <ul>
 *      <li> ENUM_CONTINUE
 *      <li> STOP_IF_BREAK if callback returns ENUM_BREAK then function executing is break
 * </ul>
 */
void rbtree_select (rbtree_t *tree, void *key, tree_item_h on_item, void *userdata, int flags);

/** @brief removes key from tree
 * @param z tree item
 */
void rbtree_del_node (tree_item_t *z);

/** @brief removes all item with key
 * @param tree
 * @param key
 */
void rbtree_del_key (rbtree_t *tree, void *key);

/** @brief executes callback \b on_item for each item
 * @param tree
 * @param ns starting item, can be NULL for all items
 * @param on_item callback function
 * @param userdata
 * @param flags
 */
void rbtree_enum (rbtree_t *tree, tree_item_t *ns, tree_item_h on_item, void *userdata, int flags);

/** @brief convert tree to list
 * @param tree
 * @return list structure
 */
list_t *rbtree_to_list (rbtree_t *tree);

/** @brief free tree structure
 * @param tree
 */
void rbtree_free (rbtree_t *tree);

#endif //  __LIBEX_TREE_H__
