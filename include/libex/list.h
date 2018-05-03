#ifndef __LIBEX_LIST_H__
#define __LIBEX_LIST_H__

#include <stdlib.h>
#include <stdio.h>

#define LIST_NOT_FOUND 0
#define LIST_FOUND 1
#define LIST_FOUND_FIN 2

#define ENUM_CONTINUE 0x00000000
#define ENUM_BREAK 0x00000001
#define ENUM_STOP_IF_BREAK ENUM_BREAK

typedef struct list list_t;
typedef struct list_item {
    void *ptr;
    struct list_item *next, *prev;
    list_t *list;
} list_item_t;

typedef int (*compare_h) (void*, void*);
typedef void* (*copy_h) (void*);
typedef void (*free_h) (void*, void*);
typedef int (*list_item_h) (list_item_t*, void*);

struct list {
    size_t len;
    struct list_item *head;
    free_h on_free;
};

list_t *lst_alloc (free_h on_free);
void lst_clear (list_t *list);
void lst_free (list_t *list);
list_item_t *lst_add (list_t *list, void *x);
list_item_t *lst_adde (list_t *list, void *x);
list_t *lst_addelst (list_t *dst, list_t *src);
list_item_t *lst_del (list_item_t *item);
void lst_enum (list_t *list, list_item_h fn, void *userdata, int flags);
list_item_t *lst_get (list_t *list, compare_h fn, void *userdata);

static inline void on_any_free_item (void *x, void *y) {
    free(x);
    if (y) free(y);
}

#define lalloc(x) lst_alloc(x)
#define lclear(x) lst_clear(x)
#define lfree(x) lst_free(x)
#define ladd(x,y) lst_add(x,y)
#define ladde(x,y) lst_adde(x,y)
#define ldel(x) lst_del(x)
#define lenum(x,y,z,n) lst_enum(x,y,z,n)
#define lget(x,y,z) lst_get(x,y,z)

#endif
