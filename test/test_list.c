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
    list_t *lst = lalloc(fn_free);
    char **p = sa;
    while (*p) {
        str_t *str = stralloc(8, 8);
        strnadd(&str, *p, strlen(*p));
        str->ptr[str->len] = '\0';
        ladd(lst, str);
        ++p;
    }
    lst_enum(lst, fn_print, NULL, 0);
    lst_clear(lst);
    printf("\n");
    p = sa;
    while (*p) {
        str_t *str = stralloc(8, 8);
        strnadd(&str, *p, strlen(*p));
        str->ptr[str->len] = '\0';
        ladde(lst, str);
        ++p;
    }
    lenum(lst, fn_print, NULL, 0);
    lfree(lst);
}

void test_url (const char *url) {
    if (!url || *url++ != '/') return;
    list_t *ret = lalloc(fn_free);
    char *q;
    while ((q = strchr(url, '/'))) {
        size_t len = (uintptr_t)q - (uintptr_t)url;
        char *p = strndup(url, len);
        ladde(ret, p);
        url += len;
        while (*url == '/') ++url;
    }
    if (*url)
        ladde(ret, strdup(url));
    lenum(ret, ({
        int fn (list_item_t *x, void*y) {
            printf("%s\n", (char*)x->ptr);
            return ENUM_CONTINUE;
        } fn;
    }), NULL, 0);
    lfree(ret);
}

int main (int argc, const char *argv[]) {
    test();
    if (argc > 1)
        test(argv[1]);
    return 0;
}
