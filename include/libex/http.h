#ifndef __LIBEX_HTTP_H__
#define __LIBEX_HTTP_H__

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include "str.h"

typedef struct {
    int32_t i_val;
    strptr_t s_val;
} http_pair_t;
extern http_pair_t http_status[];
int http_get_status (int status);

#define MAX_HTTP_ITEM 256
typedef struct {
    size_t len;
    strptr_t keys [MAX_HTTP_ITEM];
    strptr_t values [MAX_HTTP_ITEM];
} http_item_buf_t;

enum { CONNECTION_UNKNOWN=0, CONNECTION_CLOSE, CONNECTION_KEEP_ALIVE };
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
    http_item_buf_t query;
    http_item_buf_t headers;
    strptr_t content;
    int connection;
    size_t content_length;
    strptr_t host;
    strptr_t referer;
    strptr_t content_type;
    strptr_t upgrade;
    strptr_t sec_websock_key;
    strptr_t sec_websock_ext;
} __attribute__ ((__packed__)) http_request_t;
int http_parse_request (http_request_t *req, char *buf, size_t buf_len);
strptr_t *http_get_header (http_request_t *req, const char *hdr_name, size_t hdr_name_len);

#define HTTP_LOADED 0
#define HTTP_PARTIAL_LOADED 1
#define HTTP_ERROR -1
#define HTTP_ERROR_QUERY_TOO_LARGE -2
#define HTTP_ERROR_HEADERS_TOO_LARGE -3

#endif // __LIBEX_HTTP_H__
