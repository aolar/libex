#include "http.h"

http_item_h on_http_query = NULL,
            on_http_header = NULL;

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
        { 431, CONST_STR_INIT("Request Header Fields Too Large") },
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
static int parse_prefix (char *p, http_request_t *req) {
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

static int parse_headers (char **str, size_t *str_len, http_request_t *req, void *userdata) {
    int rc;
    strptr_t key = { .ptr = NULL, .len = 0 },
             val = { .ptr = NULL, .len = 0 };
    while (-1 != strntok(str, str_len, CONST_STR_LEN(":\r"), &key) && *str_len >= 3) {
        if (*(*str-1) == '\r' || 0 == key.len)
            return HTTP_ERROR;
        strntrim(&key.ptr, &key.len);
        if (-1 != strntok(str, str_len, CONST_STR_LEN("\r"), &val) && *str_len >= 3)
            strntrim(&val.ptr, &val.len);
        if (0 == cmpstr(key.ptr, key.len, CONST_STR_LEN("Content-Length"))) {
            char *tail;
            req->content_length = strtol(val.ptr, &tail, 0);
            if ('\r' != tail[0] || ERANGE == errno)
                return HTTP_ERROR;
        } else
        if (0 == cmpstr(key.ptr, key.len, CONST_STR_LEN("Host")))
            req->host = val;
        else
        if (0 == cmpstr(key.ptr, key.len, CONST_STR_LEN("Connection")))
            req->connection = val;
        else
        if (on_http_header && 0 > (rc = on_http_header(&key, &val, userdata)))
            return rc;
        if (*str_len == 0)
            return HTTP_PARTIAL_LOADED;
        ++*str; --*str_len;
        if (0 == cmpstr(*str, 2, CONST_STR_LEN("\r\n"))) {
            *str += 2; *str_len -= 2;
            return HTTP_LOADED;
        }
    }
    return HTTP_PARTIAL_LOADED;
}

static int parse_query (char **str, size_t *str_len, void *userdata) {
    int rc;
    strptr_t key = { .ptr = NULL, .len = 0 },
             val = { .ptr = NULL, .len = 0 };
    while (-1 != strntok(str, str_len, CONST_STR_LEN("= "), &key)) {
        if (**str != '=' || 0 == key.len)
            return HTTP_ERROR;
        strntrim(&key.ptr, &key.len);
        if (-1 != strntok(str, str_len, CONST_STR_LEN("& "), &val))
            strntrim(&val.ptr, &val.len);
        if (on_http_query && 0 > (rc = on_http_query(&key, &val, userdata)))
            return rc;
        if (*(*str-1) == ' ' && *str_len > 0)
            return HTTP_LOADED;
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

int http_parse_request (http_request_t *req, char *buf, size_t buf_len, void *userdata) {
    int ret, method_idx;
    char *str = buf, *p;
    size_t str_len = buf_len;
    strptr_t method_str;
    if (-1 == strntok(&str, &str_len, CONST_STR_LEN(" "), &method_str) || 0 == str_len)
        return HTTP_PARTIAL_LOADED;
    if (-1 == (method_idx = get_http_method(&method_str)))
        return HTTP_ERROR;
    req->method = http_methods[method_idx].i_val;
    if (-1 == strntok(&str, &str_len, CONST_STR_LEN(" ?"), &req->url) || 0 == str_len)
        return HTTP_PARTIAL_LOADED;
    if ((p = strnstr(req->url.ptr, req->url.len, CONST_STR_LEN("://"))))
        if (-1 == parse_prefix(p, req))
            return HTTP_ERROR;
    if (*(str-1) == '?') {
        if (0 == str_len)
            return HTTP_PARTIAL_LOADED;
        if (HTTP_LOADED != (ret = parse_query(&str, &str_len, userdata)) || 0 == str_len)
            return ret;
    }
    if (-1 == strntok(&str, &str_len, CONST_STR_LEN("\r"), &req->ver) || 0 == str_len)
        return HTTP_PARTIAL_LOADED;
    if (*str != '\n')
        return HTTP_PARTIAL_LOADED;
    ++str; --str_len;
    if (HTTP_LOADED != (ret = parse_headers(&str, &str_len, req, userdata)))
        return ret;
    if (0 != str_len) {
        req->content.ptr = str;
        req->content.len = str_len;
    }
    return HTTP_LOADED;
}

int http_cmpurl (const char *url, size_t url_len, const char *url_ep, size_t url_ep_len) {
    if (url_len > 1 && url[url_len - 1] == '/')
        url_len--;
    if (url_len != url_ep_len)
        return -1;
    return 0 == cmpstr(url, url_len, url_ep, url_ep_len) ? 0 : -1;
}
