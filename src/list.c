#include "list.h"

list_t *lst_alloc (free_h on_free) {
    list_t *list = calloc(1, sizeof(list_t));
    list->on_free = on_free;
    return list;
}

void lst_clear (list_t *list) {
    while (list->head)
        lst_del(list->head);
}

void lst_free (list_t *list) {
    if (list) {
        lst_clear(list);
        free(list);
    }
}

list_item_t *lst_add (list_t *list, void *x) {
    list_item_t *item = malloc(sizeof(list_item_t));
    if (!item) return NULL;
    item->ptr = x;
    if (!list->head)
        item->next = item->prev = item;
    else {
        item->next = list->head;
        item->prev = list->head->prev;
        list->head->prev = item;
        item->prev->next = item;
    }
    list->head = item;
    item->list = list;
    ++list->len;
    return item;
}

list_item_t *lst_adde (list_t *list, void *x) {
    list_item_t *item = malloc(sizeof(list_item_t));
    if (!item) return NULL;
    item->ptr = x;
    if (!list->head) {
        item->next = item->prev = item;
        list->head = item;
    } else {
        item->next = list->head;
        item->prev = list->head->prev;
        list->head->prev = item;
        item->prev->next = item;
    }
    item->list = list;
    ++list->len;
    return item;
}

static int on_addlst (list_item_t *li, list_t *dst) {
    lst_adde(dst, li->ptr);
    return ENUM_CONTINUE;
}

list_t *lst_addelst (list_t *dst, list_t *src) {
    lst_enum(src, (list_item_h)on_addlst, (void*)dst, 0);
    return dst;
}

list_item_t *lst_del (list_item_t *item) {
    list_item_t *x = item, *prev = item->prev, *next = item->next;
    list_t *list = item->list;
    if (prev) prev->next = next;
    if (next) next->prev = prev;
    if (x == list->head) list->head = next;
    if (list->on_free)
        list->on_free(item->ptr, NULL);
    free(item);
    --list->len;
    if (0 == list->len) next = list->head = NULL;
    return next;
}

int lst_enum (list_t *list, list_item_h fn, void *userdata, int flags) {
    list_item_t *x = list->head;
    if (x) {
        do {
            int n = fn(x, userdata);
            if (ENUM_BREAK == n && ENUM_STOP_IF_BREAK == flags)
                return ENUM_BREAK;
            x = x->next;
        } while (x != list->head);
    }
    return ENUM_CONTINUE;
}

list_item_t *lst_get (list_t *list, compare_h fn, void *userdata) {
    list_item_t *li;
    if ((li = list->head)) {
        do {
            if (0 == fn(li->ptr, userdata))
                return li;
            li = li->next;
        } while (li != list->head);
    }
    return NULL;
}
