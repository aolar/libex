#include "html.h"

#define FOUND_NONE 0
#define FOUND_TAG 1
#define FOUND_TEXT 2
#define TAG_START 0x0001
#define TAG_FIN 0x0002

static void html_clear_tag (html_tag_t *tag) {
    if (tag->tag_s) free(tag->tag_s);
    if (tag->tag_t) free(tag->tag_t);
    if (tag->tag_e) free(tag->tag_e);
    lfree(tag->tags);
}

html_tag_t *html_tag_alloc (html_tag_t *parent, int add_to_head) {
    html_tag_t *ntag = calloc(1, sizeof(html_tag_t));
    ntag->tags = lalloc((free_h)html_clear_tag);
    if (parent) {
        if (add_to_head) ladd(parent->tags, ntag); else ladde(parent->tags, ntag);
    }
    ntag->parent = parent;
    return ntag;
}

void html_free_tag (html_tag_t *tag) {
    html_clear_tag(tag);
    free(tag);
}

static int html_free_tag_cb (html_tag_t *tag, void *userdata) {
    html_free_tag(tag);
    return ENUM_CONTINUE;
}

void html_clear_tags (list_t *tags) {
    if (tags) lenum(tags, (list_item_h)html_free_tag_cb, NULL, ENUM_CONTINUE);
    while (tags->head) ldel(tags->head);
}

void html_clear (html_t *html) {
    html_free_tag(html->head);
}

static int is_autoclose_tag (strptr_t *tag_name) {
    if (0 == cmpstr(tag_name->ptr, tag_name->len, CONST_STR_LEN("meta"))) return 1;
    if (0 == cmpstr(tag_name->ptr, tag_name->len, CONST_STR_LEN("link"))) return 1;
    if (0 == cmpstr(tag_name->ptr, tag_name->len, CONST_STR_LEN("br"))) return 1;
    if (0 == cmpstr(tag_name->ptr, tag_name->len, CONST_STR_LEN("input"))) return 1;
    if (0 == cmpstr(tag_name->ptr, tag_name->len, CONST_STR_LEN("img"))) return 1;
    if (0 == cmpcasestr(tag_name->ptr, tag_name->len, CONST_STR_LEN("!doctype"))) return 1;
    return 0;
}

int html_get_next (char **ptr, size_t *clen) {
    size_t l = *clen;
    char *p = *ptr;
    while (l > 0 && isspace(*p)) { --l; ++p; }
    if (0 == l) return FOUND_NONE;
    if (*p == '<') {
        *ptr = p; *clen = l;
        return FOUND_TAG;
    }
    return FOUND_TEXT;
}

int html_get_tag (char **ptr, size_t *clen, str_t **tag) {
    int flags = TAG_START;
    char *p = strnchr(*ptr, '>', *clen);
    if (!p || *(p-1) == '<') return -1;
    if (*(p-1) == '/') {
        if (*(p-2) == '<') return -1;
        flags |= TAG_FIN;
    } else
    if (*(*ptr+1) == '/')
        flags = TAG_FIN;
    ++p;
    if (!(*tag = mkstr(*ptr, (uintptr_t)p - (uintptr_t)*ptr, 32))) return -1;
    *clen -= (*tag)->len;
    *ptr = p;
    return flags;
}

int html_get_text (char **ptr, size_t *clen, str_t **tag) {
    char *p = *ptr, *e = p + *clen;
    while (p < e && *p != '<') {
        if (*p == '"') {
            ++p;
            while (p < e && *p != '"') ++p;
        }
        ++p;
    }
    if (p >= e) return -1;
    if (!(*tag = mkstr(*ptr, (uintptr_t)p - (uintptr_t)*ptr, 32))) return -1;
    *ptr = p;
    *clen -= (*tag)->len;
    return 0;
}

int html_get_tag_attr (str_t *tag, int idx, strptr_t *val) {
    if (!tag->ptr) return -1;
    char *p = tag->ptr+1, *q;
    size_t l = tag->len-2;
    for (int i = 0; i < idx; ++i) {
        while (l > 0) {
            while (l > 0 && !isspace(*p)) { --l; ++p; }
            if (l > 0 && *p == '"') {
                --l; ++p;
                while (l > 0 && *p != '"') { --l; ++p; }
            }
            if (0 == l) return -1;
            --l; ++p;
        }
        while (l > 0 && isspace(*p)) { --l; ++p; }
    }
    if (0 == l) return -1;
    q = p;
    while (l > 0 && !isspace(*q)) {
        if (*q == '"') {
            --l; ++p;
            while (l > 0 && *q != '"') { --l; ++p; };
            if (0 == l) return -1;
        }
        --l; ++q;
    }
    val->ptr = p;
    val->len = (uintptr_t)q - (uintptr_t)p;
    return val->len > 0 ? 0 : -1;
}

int html_get_tags (char **ptr, size_t *clen, html_tag_t *parent) {
    int ret;
    while (0 < (ret = html_get_next(ptr, clen))) {
        if (FOUND_TAG == ret) {
            str_t *tag;
            int type;
            if (-1 == (type = html_get_tag(ptr, clen, &tag))) { ret = -1; break; }
            if ((type & TAG_START)) {
                strptr_t tag_name;
                html_tag_t *ntag = html_tag_alloc(parent, 0);
                ntag->tag_s = tag;
                if (-1 == (ret = html_get_tag_attr(tag, 0, &tag_name))) break;
                if (!is_autoclose_tag(&tag_name) && !(type & TAG_FIN)) {
                    if (-1 == (ret = html_get_tags(ptr, clen, ntag))) break;
                }
            } else
            if ((type & TAG_FIN)) {
                parent->tag_e = tag;
                break;
            }
        } else
        if (FOUND_TEXT == ret) {
            str_t *tag;
            if (-1 == (ret = html_get_text(ptr, clen, &tag))) break;
            html_tag_t *ntag = html_tag_alloc(parent, 0);
            ntag->tag_t = tag;
        }
    }
    return ret;
}

int html_parse (char *content, size_t content_length, html_t *html) {
    memset(html, 0, sizeof(html_t));
    html->head = html_tag_alloc(NULL, 0);
    while (content_length > 0 && isspace(*content)) {
        --content_length;
        ++content;
    }
    return html_get_tags(&content, &content_length, html->head);
}

int html_add_tag (html_tag_t *parent,
                  const char *tag_s, size_t tag_s_len,
                  const char *tag_t, size_t tag_t_len,
                  const char *tag_e, size_t tag_e_len,
                  int add_to_head) {
    html_tag_t *ntag = html_tag_alloc(parent, add_to_head);
    if (tag_s && tag_s_len && !(ntag->tag_s = mkstr(tag_s, tag_s_len, 32))) return -1;
    if (tag_t && tag_t_len && !(ntag->tag_t = mkstr(tag_t, tag_t_len, 32))) return -1;
    if (tag_e && tag_e_len && !(ntag->tag_e = mkstr(tag_e, tag_e_len, 32))) return -1;
    return 0;
}

int html_find_tag_intr (list_t *tags, list_item_t *item, on_tag_h on_tag, int recurse, int here, list_item_t **found_item, void *userdata) {
    if (!item) return HTML_NOT_FOUND;
    do {
        list_item_t *f_item = on_tag(item, userdata);
        if (f_item) {
            if (found_item) *found_item = f_item;
            if (item->next == tags->head && here) return HTML_FOUND_FIN;
            return HTML_FOUND;
        } else
        if (recurse) {
            html_tag_t *tag = (html_tag_t*)item->ptr;
            int ret = html_find_tag_intr(tag->tags, tag->tags->head, on_tag, recurse, 0, found_item, userdata);
            if (HTML_NOT_FOUND != ret) return ret;
        }
        item = item->next;
    } while (item != tags->head);
    return HTML_NOT_FOUND;
}

int html_find_tag (list_t *tags, list_item_t *item, on_tag_h on_tag, int recurse, list_item_t **found_item, void *userdata) {
    int ret;
    if (!item && !(item = tags->head)) return HTML_NOT_FOUND;
    ret = html_find_tag_intr(tags, item, on_tag, recurse, 1, found_item, userdata);
    if (HTML_NOT_FOUND != ret && (*found_item)->next == tags->head) ret = HTML_FOUND_FIN;
    return ret;
}

void html_mktags (html_tag_t *tag, str_t **content) {
    list_item_t *item = tag->tags->head;
    if (tag->tag_s) strnadd(content, tag->tag_s->ptr, tag->tag_s->len);
    if (tag->tag_t) strnadd(content, tag->tag_t->ptr, tag->tag_t->len);
    if (item) {
        do {
            html_tag_t *itag = (html_tag_t*)item->ptr;
            html_mktags(itag, content);
            item = item->next;
        } while (item != tag->tags->head);
    }
    if (tag->tag_e) strnadd(content, tag->tag_e->ptr, tag->tag_e->len);
}

str_t *html_mkcontent (html_t *html, size_t start_len, size_t chunk_size) {
    str_t *content = stralloc(start_len, chunk_size);
    if (html->head) html_mktags(html->head, &content);
    return content;
}
