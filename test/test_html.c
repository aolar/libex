#include <stdio.h>
#include <sys/stat.h>
#include "../include/libex/html.h"

void p_str (str_t *s, int l) {
    if (s && s->len > 0) {
        char b [l+1];
        memset(&b, '\t', l);
        b[l] = '\0';
        STR_ADD_NULL(s);
        printf("%s%s", b, s->ptr);
    }
}

void print_tag (html_tag_t *tag, int level);
void print_tags (list_t *tags, int level) {
    list_item_t *item = tags->head;
    if (item) {
        printf("\n");
        do {
            print_tag((html_tag_t*)item->ptr, level);
            item = item->next;
        } while (item != tags->head);
    }
}

void print_tag (html_tag_t *tag, int level) {
    p_str(tag->tag_s, level);
    print_tags(tag->tags, level+1);
    p_str(tag->tag_e, level);
    printf("\n");
}

char *load_html (const char *fname) {
    FILE *f = fopen(fname, "r");
    char *ret = NULL;
    if (f) {
        struct stat st;
        stat(fname, &st);
        ret = malloc(st.st_size);
        fread(ret, st.st_size, 1, f);
        fclose(f);
    }
    return ret;
}

int main (int argc, const char *argv[]) {
    html_t html;
    char *str = load_html(argv[1]);
    int len = strlen(str);
    if (!str) return 1;
    if (0 == html_parse(str, len, &html)) {
        print_tag(html.head, 0);
    }
    str_t *content = html_mkcontent(&html, 1024, 1024);
    STR_ADD_NULL(content);
    printf("----------\n");
    printf("%s\n", content->ptr);
    free(content);
    html_clear(&html);
    free(str);
    return 0;
}
