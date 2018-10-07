#include <stdlib.h>
#include "http.h"

#define HTTP1 "POST /news.html HTTP/1.0\r\n" \
    "Host: www.site.ru\r\n" \
    "Referer: http://www.site.ru/index.html\r\n" \
    "Cookie: income=1\r\n" \
    "Content-Type: application/x-www-form-urlencoded\r\n" \
    "Content-Length: 35\r\n" \
    "\r\n" \
    "login=Petya&password=qq"

void prnt (const char *name, strptr_t *str) {
    str_t *s = mkstr(str->ptr, str->len, 8); STR_ADD_NULL(s);
    printf("%s: %s\n", name, s->ptr);
    free(s);
}

void prntl (strptr_t *s1, strptr_t *s2) {
    str_t *s = mkstr(s1->ptr, s1->len, 8); STR_ADD_NULL(s);
    printf("\t%s: ", s->ptr);
    strput(&s, s2->ptr, s2->len, 0); STR_ADD_NULL(s);
    printf("\t%s\n", s->ptr);
    free(s);
}

void test_http (char *content, size_t content_len) {
    http_request_t req;
    memset(&req, 0, sizeof req);
    int ret = http_parse_request(&req, content, content_len);
    if (HTTP_LOADED == ret) {
        printf("method: %d\n", req.method);
        prnt("url", &req.url);
        prnt("ver", &req.ver);
        if (req.query.len > 0) {
            printf("query string:\n");
            for (size_t i = 0; i < req.query.len; ++i)
                prntl(&req.query.keys[i], &req.query.values[i]);
        }
        if (req.headers.len > 0) {
            printf("headers:\n");
            for (size_t i = 0; i < req.headers.len; ++i)
                prntl(&req.headers.keys[i], &req.headers.values[i]);
        }
        if (req.content.len)
            prnt("\n", &req.content);
        printf("\ncustom headers:\n");
        printf("connection: %d\n", req.connection);
        if (req.host.len)
            prnt("host", &req.host);
        if (req.referer.len)
            prnt("referer", &req.referer);
        if (req.content_type.len)
            prnt("content type", &req.content_type);
    }
    printf("result: %d\n", ret);
}

int main () {
    test_http(CONST_STR_LEN(HTTP1));
    return 0;
}
