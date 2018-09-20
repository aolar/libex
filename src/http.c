#include "http.h"

http_pair_t http_status[] = {
        { 100, CONST_STR_INIT("Continue") },
        { 101, CONST_STR_INIT("Switching Protocols") },
        { 102, CONST_STR_INIT("Processing") }, /* WebDAV */
        { 200, CONST_STR_INIT("OK") },
        { 201, CONST_STR_INIT("Created") },
        { 202, CONST_STR_INIT("Accepted") },
        { 203, CONST_STR_INIT("Non-Authoritative Information") },
        { 204, CONST_STR_INIT("No Content") },
        { 205, CONST_STR_INIT("Reset Content") },
        { 206, CONST_STR_INIT("Partial Content") },
        { 207, CONST_STR_INIT("Multi-status") }, /* WebDAV */
        { 300, CONST_STR_INIT("Multiple Choices") },
        { 301, CONST_STR_INIT("Moved Permanently") },
        { 302, CONST_STR_INIT("Found") },
        { 303, CONST_STR_INIT("See Other") },
        { 304, CONST_STR_INIT("Not Modified") },
        { 305, CONST_STR_INIT("Use Proxy") },
        { 306, CONST_STR_INIT("(Unused)") },
        { 307, CONST_STR_INIT("Temporary Redirect") },
        { 400, CONST_STR_INIT("Bad Request") },
        { 401, CONST_STR_INIT("Unauthorized") },
        { 402, CONST_STR_INIT("Payment Required") },
        { 403, CONST_STR_INIT("Forbidden") },
        { 404, CONST_STR_INIT("Not Found") },
        { 405, CONST_STR_INIT("Method Not Allowed") },
        { 406, CONST_STR_INIT("Not Acceptable") },
        { 407, CONST_STR_INIT("Proxy Authentication Required") },
        { 408, CONST_STR_INIT("Request Timeout") },
        { 409, CONST_STR_INIT("Conflict") },
        { 410, CONST_STR_INIT("Gone") },
        { 411, CONST_STR_INIT("Length Required") },
        { 412, CONST_STR_INIT("Precondition Failed") },
        { 413, CONST_STR_INIT("Request Entity Too Large") },
        { 414, CONST_STR_INIT("Request-URI Too Long") },
        { 415, CONST_STR_INIT("Unsupported Media Type") },
        { 416, CONST_STR_INIT("Requested Range Not Satisfiable") },
        { 417, CONST_STR_INIT("Expectation Failed") },
        { 422, CONST_STR_INIT("Unprocessable Entity") }, /* WebDAV */
        { 423, CONST_STR_INIT("Locked") }, /* WebDAV */
        { 424, CONST_STR_INIT("Failed Dependency") }, /* WebDAV */
        { 426, CONST_STR_INIT("Upgrade Required") }, /* TLS */
        { 429, CONST_STR_INIT("Too Many Requests") },
        { 500, CONST_STR_INIT("Internal Server Error") },
        { 501, CONST_STR_INIT("Not Implemented") },
        { 502, CONST_STR_INIT("Bad Gateway") },
        { 503, CONST_STR_INIT("Service Not Available") },
        { 504, CONST_STR_INIT("Gateway Timeout") },
        { 505, CONST_STR_INIT("HTTP Version Not Supported") },
        { 507, CONST_STR_INIT("Insufficient Storage") } /* WebDAV */
};

int http_get_status (int status) {
    int l = 0, r = (sizeof(http_status) / sizeof(http_pair_t)), found = 0, m;
    while (l <= r) {
        m = (l + r) / 2;
        if (status > http_status[m].i_val) l = m + 1; else
        if (status < http_status[m].i_val) r = m - 1; else {
            found = 1; break;
        }
    }
    return found ? m : -1;
}


#define MAX_HEADERS_LEN 32768
#define MAX_QUERY_LEN 8192
static int parse_prefix (char *p, http_request_buf_t *req) {
    char *q;
    req->prot.ptr = req->url.ptr;
    req->prot.len = (uintptr_t)p - (uintptr_t)req->url.ptr;
    p += sizeof("://")-1;
    if (!(q = strnchr(p, '/', req->url.len - req->prot.len - sizeof("://")-1))) return -1;
    req->domain.ptr = p;
    req->domain.len = (uintptr_t)q - (uintptr_t)p;
    req->url.ptr = q;
    req->url.len -= req->prot.len + req->domain.len + sizeof("://")-1;
    if ((p = strnchr(req->domain.ptr, ':', req->domain.len))) {
        size_t port_len = (uintptr_t)(req->domain.ptr + req->domain.len) - (uintptr_t)p - 1;
        if (0 == port_len) return -1;
        req->domain.len = (uintptr_t)p - (uintptr_t)req->domain.ptr;
        req->port.ptr = p + 1;
        req->port.len = port_len;
    }
    return 0;
}

static int set_request_header (strptr_t *key, strptr_t *value, http_request_buf_t *req) {
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN("Connection"))) {
        if (0 == cmpstr(value->ptr, value->len, CONST_STR_LEN("close"))) req->connection = CONNECTION_CLOSE; else
        if (0 == cmpstr(value->ptr, value->len, CONST_STR_LEN("keep-alive"))) req->connection = CONNECTION_KEEP_ALIVE; else
            return -1;
    } else
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN("Content-Length"))) {
        if (value->len > 16) return -1;
        char buf [16], *tail;
        strncpy(buf, value->ptr, sizeof buf); buf[value->len] = '\0';
        req->content_length = strtol(buf, &tail, 0);
        if (*tail != '\0' || ERANGE == errno) return -1;
    } else
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN("Host"))) { req->host.ptr = value->ptr; req->host.len = value->len; } else
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN("Referer"))) { req->referer.ptr = value->ptr; req->referer.len = value->len; } else
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN("Content-Type"))) { req->content_type.ptr = value->ptr; req->content_type.len = value->len; }
    return 0;
}

static int parse_headers (char **str, size_t *str_len, http_item_buf_t *headers, http_request_buf_t *req) {
    size_t i = headers->len = 0;
    while (0 == strntok(str, str_len, CONST_STR_LEN(":\r"), &headers->keys[i]) && *str_len >= 3) {
        if (*(*str-1) == '\r' || 0 == headers->keys[i].len) return HTTP_ERROR;
        strntrim(&headers->keys[i].ptr, &headers->keys[i].len);
        if (0 == strntok(str, str_len, CONST_STR_LEN("\r"), &headers->values[i]) && *str_len >= 3)
            strntrim(&headers->values[i].ptr, &headers->values[i].len);
        if (-1 == set_request_header(&headers->keys[i], &headers->values[i], req)) return HTTP_ERROR;
        if (++i >= MAX_HTTP_ITEM) return HTTP_ERROR_HEADERS_TOO_LARGE;
        headers->len = i;
        ++*str; --*str_len;
        if (0 == cmpstr(*str, 2, CONST_STR_LEN("\r\n"))) {
            *str += 2; *str_len -= 2;
            if (CONNECTION_UNKNOWN == req->connection) req->connection = CONNECTION_CLOSE;
            return HTTP_LOADED;
        }
    }
    return HTTP_PARTIAL_LOADED;
}

static int parse_query (char **str, size_t *str_len, http_item_buf_t *query) {
    size_t i = query->len = 0;
    while (0 == strntok(str, str_len, CONST_STR_LEN("= "), &query->keys[i])) {
        if (**str != '=' || 0 == query->keys[i].len) return HTTP_ERROR;
        strntrim(&query->keys[i].ptr, &query->keys[i].len);
        if (0 == strntok(str, str_len, CONST_STR_LEN("& "), &query->values[i]))
            strntrim(&query->values[i].ptr, &query->values[i].len);
        if (++i >= MAX_HTTP_ITEM) return HTTP_ERROR_QUERY_TOO_LARGE;
        query->len = i;
        if (*(*str-1) == ' ' && *str_len > 0) return HTTP_LOADED;
    }
    return HTTP_PARTIAL_LOADED;
}

http_pair_t http_methods [] = {
    {HTTP_OPTIONS,CONST_STR_INIT("OPTIONS")}, {HTTP_GET,CONST_STR_INIT("GET")}, {HTTP_HEAD,CONST_STR_INIT("HEAD")},
    {HTTP_POST,CONST_STR_INIT("POST")}, {HTTP_PUT,CONST_STR_INIT("PUT")}, {HTTP_PATCH,CONST_STR_INIT("PATCH")},
    {HTTP_DELETE,CONST_STR_INIT("DELETE")}, {HTTP_TRACE,CONST_STR_INIT("TRACE")}, {HTTP_LINK,CONST_STR_INIT("LINK")},
    {HTTP_UNLINK,CONST_STR_INIT("UNLINK")}, {HTTP_CONNECT,CONST_STR_INIT("CONNECT")}
};

static int get_http_method (strptr_t *str) {
    for (int i = 0; i < sizeof(http_methods)/sizeof(http_pair_t); ++i) {
        if (0 == cmpstr(http_methods[i].s_val.ptr, http_methods[i].s_val.len, str->ptr, str->len))
            return i;
    }
    return -1;
}

int http_parse_request (http_request_buf_t *req, char *buf, size_t buf_len) {
    int ret, method_idx;
    char *str = buf, *p;//req->buf->ptr, *p;
    size_t str_len = buf_len;//req->buf->len;
    strptr_t method_str;
    if (0 != strntok(&str, &str_len, CONST_STR_LEN(" "), &method_str) || 0 == str_len) return HTTP_PARTIAL_LOADED;
    if (-1 == (method_idx = get_http_method(&method_str))) return HTTP_ERROR;
    req->method = http_methods[method_idx].i_val;
    if (0 != strntok(&str, &str_len, CONST_STR_LEN(" ?"), &req->url) || 0 == str_len) return HTTP_PARTIAL_LOADED;
    if ((p = strnstr(req->url.ptr, req->url.len, CONST_STR_LEN("://")))) // TODO : in start string ?
        if (-1 == parse_prefix(p, req)) return HTTP_ERROR;
    if (*(str-1) == '?') {
        if (0 == str_len) return HTTP_PARTIAL_LOADED;
        if (HTTP_LOADED != (ret = parse_query(&str, &str_len, &req->query)) || 0 == str_len) return ret;
    }
    if (0 != strntok(&str, &str_len, CONST_STR_LEN("\r"), &req->ver) || 0 == str_len) return HTTP_PARTIAL_LOADED;
    if (*str != '\n') return HTTP_PARTIAL_LOADED;
    ++str; --str_len;
    if (HTTP_LOADED != (ret = parse_headers(&str, &str_len, &req->headers, req))) return ret;
    if (0 != str_len) {
        req->content.ptr = str;
        req->content.len = str_len;
    }
    return HTTP_LOADED;
}

strptr_t *http_get_header (http_request_buf_t *req, const char *hdr_name, size_t hdr_name_len) {
    for (size_t i = 0; i < req->headers.len; ++i)
        if (0 == cmpstr(req->headers.keys[i].ptr, req->headers.keys[i].len, hdr_name, hdr_name_len))
            return &req->headers.values[i];
    return NULL;
}

int http_protocol_next (const char **url, strptr_t *result) {
    const char *p = *url;
    if ('\0' == *p) return -1;
    while (*p && *p != '/') ++p;
    if (p > *url && *(p-1) == ':' && *(++p) == '/') {
        result->ptr = (char*)*url;
        result->len = (uintptr_t)p - (uintptr_t)*url - 2;
        *url = p;
        return 1;
    }
    return 0;
}

int http_domain_next (const char **url, strptr_t *result) {
    const char *p = *url, *e;
    size_t len;
    if ('/' == *p) ++p;
    e = p;
    while (*e && '/' != *e && ':' != *e) ++e;
    if (!(len = (uintptr_t)e - (uintptr_t)p))
        return -1;
    result->ptr = (char*)p;
    result->len = len;
    *url = e;
    return 1;
}

int http_port_next (const char **url, strptr_t *result) {
    const char *p = *url, *e;
    size_t len;
    if ('\0' == *p) return -1;
    if (':' != *p) return 0;
    *url = e = ++p;
    while (*e && *e != '/') ++e;
    if ('\0' == *e) return -1;
    if (!(len = (uintptr_t)e - (uintptr_t)p))
        return -1;
    result->ptr = (char*)p;
    result->len = len;
    *url = e;
    return 1;
}

int http_url_next_part (const char **url, strptr_t *result) {
    const char *p_url, *e_url;
    size_t len;
    if ('\0' == **url) return -1;
    p_url = *url;
    if ('/' == *p_url) ++p_url;
    if ('/' == *p_url) ++p_url;
    if (result)
        result->ptr = (char*)p_url;
    e_url = p_url;
    while (*e_url && '/' != *e_url) ++e_url;
    if (!(len = ((uintptr_t)e_url - (uintptr_t)p_url)))
        return 0;
    if (result)
        result->len = (uintptr_t)e_url - (uintptr_t)p_url;
    *url = e_url;
    return 1;
}
