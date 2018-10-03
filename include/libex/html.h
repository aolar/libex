#ifndef __LIBEX_HTML_H__
#define __LIBEX_HTML_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "str.h"
#include "list.h"

#define HTML_NOT_FOUND -1
#define HTML_FOUND 0
#define HTML_FOUND_FIN 1

typedef list_item_t* (*on_tag_h) (list_item_t*, void*);

typedef struct html_tag {
    str_t *tag_s;
    str_t *tag_t;
    str_t *tag_e;
    list_t *tags;
    struct html_tag *parent;
} html_tag_t;

typedef struct {
    html_tag_t *head;
} html_t;

void html_clear (html_t *html);
void html_free_tag (html_tag_t *tag);
void html_clear_tags (list_t *tags);
int html_parse (char *content, size_t content_length, html_t *html);
int html_add_tag (html_tag_t *parent,
                  const char *tag_s, size_t tag_s_len,
                  const char *tag_t, size_t tag_t_len,
                  const char *tag_e, size_t tag_e_len,
                  int add_to_head);
int html_find_tag (list_t *tags, list_item_t *item, on_tag_h on_tag, int recurse, list_item_t **found_item, void *userdata);
int html_get_tag_attr (str_t *tag, int idx, strptr_t *val);
str_t *html_mkcontent (html_t *html, size_t start_len, size_t chunk_size);

#endif // __LIBEX_HTML_H__
