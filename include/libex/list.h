/**
 * @file list.h
 * @brief list functions
 */
#ifndef __LIBEX_LIST_H__
#define __LIBEX_LIST_H__

#include <stdlib.h>
#include <stdio.h>

/** list item not found */
#define LIST_NOT_FOUND 0
/** list item found */
#define LIST_FOUND 1
/** list item found and it the last item */
#define LIST_FOUND_FIN 2

/** continue enumeration */
#define ENUM_CONTINUE 0x00000000
/** break emumeration */
#define ENUM_BREAK 0x00000001
/** stop enumeration if callback function returns #ENUM_BREAK */
#define ENUM_STOP_IF_BREAK ENUM_BREAK

/** opaque list structure */
typedef struct list list_t;
/** list item structure */
typedef struct list_item {
    void *ptr;                  /**< data pointer */
    struct list_item *next;     /**< next item */
    struct list_item *prev;     /**< previous item */
    list_t *list;               /**< list */
} list_item_t;

/** callback for compare two items */
typedef int (*compare_h) (void*, void*);
/** callback for copy data */
typedef void* (*copy_h) (void*);
/** callback for free item */
typedef void (*free_h) (void*, void*);
/** callback for searching item */
typedef int (*list_item_h) (list_item_t*, void*);

/** @brief list structure */
struct list {
    size_t len;                 /**< items count */
    struct list_item *head;     /**< head */
    free_h on_free;             /**< callback for free item data */
};

/** @brief create list
 * @param on_free
 */
list_t *lst_alloc (free_h on_free);

/** @brief clear list
 * @param list
 */
void lst_clear (list_t *list);

/** @brief free list
 * @param list
 */
void lst_free (list_t *list);

/** @brief add item to head of list
 * @param list
 * @param x pointer to user data
 * @return list item structure
 */
list_item_t *lst_add (list_t *list, void *x);

/** @brief add item to end of list
 * @param list
 * @param x pointer to user data
 * @return list item structure
 */
list_item_t *lst_adde (list_t *list, void *x);

/** @brief add list to end of list
 * @param dst destination list
 * @param src source list
 * @return list
 */
list_t *lst_addelst (list_t *dst, list_t *src);

/** @brief remove item from list
 * @param item
 */
list_item_t *lst_del (list_item_t *item);

/** @brief execute callback function for each item
 * @param list
 * @param fn
 * @param userdata
 * @param flags
 * <ul>
 *      <li> ENUM_CONTINUE
 *      <li> ENUM_STOP_IF_BREAK
 * </ul>
 */
void lst_enum (list_t *list, list_item_h fn, void *userdata, int flags);

/** @brief get item for returning callback function
 * @param list
 * @param fn
 * @param userdata
 * @return list item
 */
list_item_t *lst_get (list_t *list, compare_h fn, void *userdata);

/** @brief default callback for free item
 * @param x
 * @param y
 */
static inline void on_default_free_item (void *x, void *y) {
    free(x);
    if (y) free(y);
}

#endif
