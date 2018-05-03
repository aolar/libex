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
    http_request_buf_t req;
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

static void print_sptr (const char *prompt, strptr_t *s) {
    char *ss = strndup(s->ptr, s->len);
    if (prompt) printf("%s", prompt);
    printf("%s\n", ss);
    free(ss);
}

static void parse_http_url (const char *url) {
    int r;
    strptr_t s = { .ptr = NULL, .len = 0 };
    printf("source: %s\n", url);
    if (-1 != (r = http_protocol_next(&url, &s))) {
        if (1 == r)
            print_sptr("protocol: ", &s);
        if (1 == r && 1 != (r = http_domain_next(&url, &s)))
            return;
        if (1 == r)
            print_sptr("domain: ", &s);
        if (1 == r && -1 == (r = http_port_next(&url, &s)))
            return;
        if (1 == r)
            print_sptr("port: ", &s);
        while (0 < http_url_next_part(&url, &s))
            print_sptr(NULL, &s);
    }
    printf("\n");
}

void test_http_url () {
    parse_http_url("api/v1/people/brian");
    parse_http_url("http://www.domain.org/api/v1/people/brian");
    parse_http_url("http://www.domain.org:8080/api/v1/people/brian");
    parse_http_url("http://127.0.0.1:8901/transform/PXRoq2pJLbIZZnI8Enjo%2FEivje5dHXJSBqPy4QaqvdlqqU1XwztgI%2B7waLnq%2Fz9CQA6DxXJBlrj%2B%0AOONlEKQS60II7bPLggEk15Xt%2BPTkcwaI30f2Ooqj1eIrPk%2FRq29UrOFxtcPQygyFfJRr6fuIugL6%0Art6jqcp%2BosTQEDD1QEY%3D%0A");
}


int main () {
    test_http(CONST_STR_LEN(HTTP1));
    test_http_url();
    return 0;
}
