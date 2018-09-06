#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/libex/str.h"
#include "../include/libex/list.h"

char *sa [] = {
    "bb", "aaa", "ccccc", "aaaa", "bbbb", "zzzz", "yy", "zzz", NULL
};

void fn_free (void *x, void *y) {
    free(x);
}

int fn_print (list_item_t *x, void *userdata) {
    str_t *s = (str_t*)x->ptr;
    printf("%s\n", s->ptr);
    return ENUM_CONTINUE;
}

void test () {
    list_t *lst = lst_alloc(fn_free);
    char **p = sa;
    while (*p) {
        str_t *str = stralloc(8, 8);
        strnadd(&str, *p, strlen(*p));
        str->ptr[str->len] = '\0';
        lst_add(lst, str);
        ++p;
    }
    lst_enum(lst, fn_print, NULL, 0);
    printf("foreach\n");
    LST_FOREACH(value, lst)
        printf("- %s\n", ((str_t*)value)->ptr);
    LST_END(lst)
    lst_clear(lst);
    printf("\n");
    p = sa;
    while (*p) {
        str_t *str = stralloc(8, 8);
        strnadd(&str, *p, strlen(*p));
        str->ptr[str->len] = '\0';
        lst_adde(lst, str);
        ++p;
    }
    lst_enum(lst, fn_print, NULL, 0);
    lst_free(lst);
}

void test_url (const char *url) {
    if (!url || *url++ != '/') return;
    list_t *ret = lst_alloc(fn_free);
    char *q;
    while ((q = strchr(url, '/'))) {
        size_t len = (uintptr_t)q - (uintptr_t)url;
        char *p = strndup(url, len);
        lst_adde(ret, p);
        url += len;
        while (*url == '/') ++url;
    }
    if (*url)
        lst_adde(ret, strdup(url));
    lst_enum(ret, ({
        int fn (list_item_t *x, void*y) {
            printf("%s\n", (char*)x->ptr);
            return ENUM_CONTINUE;
        } fn;
    }), NULL, 0);
    lst_free(ret);
}

int main (int argc, const char *argv[]) {
    test();
    if (argc > 1)
        test(argv[1]);
    return 0;
}
