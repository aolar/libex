#include "wsnet.h"

char wst_ping [] = {0x89, 0x05, 0xd0, 0x9a, 0xd1, 0x83, 0x3f};
char wst_close [] = {0x88, 0x04, 0xd0, 0x9a, 0xd1, 0x8e};

int wsnet_handshake (int fd) {
    int rc = -1;
    strbuf_t buf;
    http_request_t req;
    ws_handshake_t wsh;
    strbufalloc(&buf, 128, 128);
    memset(&wsh, 0, sizeof(wsh));
    memset(&req, 0, sizeof(req));
    while (net_recv(fd, &buf, NULL) > 0) {
        if (HTTP_LOADED == ws_handshake(&req, buf.ptr, buf.len, &wsh)) {
            ws_make_response(&buf, &wsh);
            if (net_write(fd, buf.ptr, buf.len, NULL) > 0)
                rc = 0;
            break;
        }
    }
    free(buf.ptr);
    return rc;
}

int wsnet_try_handshake (int fd, strbuf_t *buf) {
    int rc = -1;
    http_request_t req;
    ws_handshake_t wsh;
    if (net_recv(fd, buf, NULL) > 0) {
        if (HTTP_LOADED == (rc = ws_handshake(&req, buf->ptr, buf->len, &wsh))) {
            ws_make_response(buf, &wsh);
            if (net_write(fd, buf->ptr, buf->len, NULL) > 0)
                rc = 1;
        } else
        if (HTTP_PARTIAL_LOADED == rc)
            rc = 0;
    } else
        rc = -1;
    return rc;
}

static void wsnet_save_tail (netbuf_t *nbuf, ssize_t nbytes) {
    if (nbytes < nbuf->buf.len)
        strbufadd(&nbuf->tail, nbuf->buf.ptr + nbytes, nbuf->buf.len - nbytes);
    else
        nbuf->tail.len = 0;
    nbuf->buf.len = nbytes;
}

int wsnet_recv (int fd, netbuf_t *nbuf, ws_t **result) {
    int rc = 0;
    ssize_t nbytes;
    if ((nbytes = ws_buflen((const uint8_t*)nbuf->buf.ptr, nbuf->buf.len)) > 0) {
        *result = calloc(1, sizeof(ws_t));
        rc = ws_parse(nbuf->buf.ptr, nbytes, *result);
        wsnet_save_tail(nbuf, nbytes);
        if (WS_OK == rc)
            return WS_OK;
        free(*result);
        *result = NULL;
        return WS_WAIT;
    }
    if (*result)
        free(*result);
    if ((nbytes = net_recv(fd, &nbuf->buf, (fmt_checker_h)ws_buflen)) > 0) {
        if (-1 == (nbytes = ws_buflen((const uint8_t*)nbuf->buf.ptr, nbuf->buf.len)))
            return WS_WAIT;
        *result = calloc(1, sizeof(ws_t));
        rc = ws_parse(nbuf->buf.ptr, nbytes, *result);
        wsnet_save_tail(nbuf, nbytes);
        if (WS_OK == rc)
            return WS_OK;
        free(*result);
        *result = NULL;
        return WS_WAIT;
    } else
    if (0 == nbytes)
        return WS_WAIT;
    return WS_ERROR;
}

void wsnet_reset (netbuf_t *nbuf) {
    strbuf_t s = nbuf->buf;
    nbuf->buf = nbuf->tail;
    nbuf->tail = s;
    nbuf->tail.len = 0;
}
