#ifndef __LIBEX_HTTP_H__
#define __LIBEX_HTTP_H__

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "str.h"

#define HTTP_LOADED 0
#define HTTP_PARTIAL_LOADED 1
#define HTTP_ERROR -1
#define HTTP_ERROR_QUERY_TOO_LARGE -2
#define HTTP_ERROR_HEADERS_TOO_LARGE -3

typedef int (*http_item_h) (strptr_t*, strptr_t*, void*);
extern http_item_h on_http_header;
extern http_item_h on_http_query;

typedef struct {
    int32_t i_val;
    strptr_t s_val;
} http_pair_t;
extern http_pair_t http_status[];
int http_get_status (int status);

enum { HTTP_OPTIONS, HTTP_GET, HTTP_HEAD, HTTP_POST, HTTP_PUT, HTTP_PATCH, HTTP_DELETE, HTTP_TRACE, HTTP_LINK, HTTP_UNLINK, HTTP_CONNECT };
typedef struct http_post_buf http_post_buf_t;
typedef struct postform postform_t;
typedef struct {
    int method;
    strptr_t prot;
    strptr_t domain;
    strptr_t port;
    strptr_t url;
    strptr_t ver;
    strptr_t content;
    strptr_t connection;
    size_t content_length;
    strptr_t host;
} http_request_t;
int http_parse_request (http_request_t *req, char *buf, size_t buf_len, void *userdata);

extern http_item_h on_http_header;
extern http_item_h on_http_query;

int http_cmpurl (const char *url, size_t url_len, const char *url_ep, size_t url_ep_len);

#endif // __LIBEX_HTTP_H__
