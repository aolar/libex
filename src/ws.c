#include "ws.h"

#define WS_RESPONSE "HTTP/1.1 101 Switching Protocols\r\n" \
    "Upgrade: websocket\r\n" \
    "Connection: Upgrade\r\n" \
    "Sec-WebSocket-Accept: %s\r\n\r\n"
#define WS_RESPONSE_LEN sizeof(WS_RESPONSE)-1
#define WS_UUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
#define WS_SECKEY "Sec-WebSocket-Key"

static int on_ws_headers (strptr_t *key, strptr_t *val, void *userdata) {
    ws_handshake_t *wsh = (ws_handshake_t*)userdata;
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(WS_SECKEY))) {
        if (val->len != WS_SECKEY_LEN)
            return HTTP_ERROR;
        strncpy(wsh->sec_key, val->ptr, val->len);
    }
    return 0;
}

static void ws_genkey (ws_handshake_t *wsh) {
    sha1_t ctx;
    uint8_t sha1sum [SHA1_LEN];
    strcpy(wsh->sec_key + WS_SECKEY_LEN, WS_UUID);
    sha1_init(&ctx);
    sha1_update(&ctx, (unsigned char*)wsh->sec_key, WS_SECKEY_LEN + WS_UUID_LEN);
    sha1_done(&ctx, sha1sum);
    ssize_t n = base64_encode(sha1sum, sizeof(sha1sum), wsh->sec_key, sizeof(wsh->sec_key));
    wsh->sec_key[n] = '\0';
}

int ws_handshake (http_request_t *req, char *buf, size_t buf_len, ws_handshake_t *wsh) {
    http_item_h h = on_http_header;
    on_http_header = on_ws_headers;
    int rc = http_parse_request(req, buf, buf_len, wsh);
    if (HTTP_LOADED == rc)
        ws_genkey(wsh);
    on_http_header = h;
    return rc;
}

void ws_make_response (strbuf_t *buf, ws_handshake_t *wsh) {
    strbufsize(buf, WS_SECKEY_LEN + WS_UUID_LEN + WS_RESPONSE_LEN + 1, 0);
    buf->len = snprintf(buf->ptr, buf->len, WS_RESPONSE, wsh->sec_key);
}

ssize_t ws_buflen (const uint8_t *buf, size_t buflen) {
    ssize_t expected_len;
    if (buflen < WS_HDRLEN)
        return WS_ERROR;
    uint8_t mlen = *(uint8_t*)(buf + sizeof(uint8_t)),
            len = mlen & 0x7f;
    if (len < 126) {
        if ((mlen & 0x80)) {
            expected_len = WS_HDRLEN + WS_MASKLEN + len;
            if (expected_len <= buflen)
                return expected_len;
            return WS_ERROR;
        }
        expected_len = WS_HDRLEN + len;
        if (expected_len <= buflen)
            return expected_len;
        return WS_ERROR;
    }
    if (126 == len) {
        uint16_t blen;
        if (buflen < WS_HDRLEN + sizeof(uint16_t))
            return WS_ERROR;
        blen = htobe16(*(uint16_t*)(buf + WS_HDRLEN));
        if ((mlen & 0x80)) {
            expected_len = WS_HDRLEN + sizeof(uint16_t) + WS_MASKLEN + blen;
            if (expected_len <= buflen)
                return expected_len;
            return WS_ERROR;
        }
        expected_len = WS_HDRLEN + sizeof(uint16_t) + blen;
        if (expected_len <= buflen)
            return expected_len;
        return WS_ERROR;
    }
    uint64_t hlen;
    if (buflen < WS_HDRLEN + sizeof(uint64_t))
        return WS_ERROR;
    hlen = htobe64(*(uint64_t*)(buf + 2));
    if ((mlen & 0x80)) {
        expected_len = WS_HDRLEN + sizeof(uint64_t) + WS_MASKLEN + hlen;
        if (expected_len <= buflen)
            return expected_len;
        return WS_ERROR;
    }
    expected_len = WS_HDRLEN + sizeof(uint64_t) + hlen;
    if (expected_len <= buflen)
        return expected_len;
    return WS_ERROR;
}

int ws_parse (const char *buf, size_t buf_len, ws_t *ws) {
    if (buf_len < sizeof(wsh_t))
        return WS_WAIT;
    ws->hdr = (ws_hdr_t*)buf;
    uint8_t mlen = WS_BODY_LEN(ws), l;
    int is_mask = WS_ISMASK(ws);
    if (mlen < 126) {
        ws->type = WS_SMALL;
        ws->len = mlen;
        if (is_mask) {
            l = mlen + sizeof(wsc_small_t);
            if (buf_len < l) return WS_WAIT;
            if (buf_len > l) return WS_TOOBIG;
            ws->ptr = (uint8_t*)buf + sizeof(wsc_small_t);
            ws->mask = ws->hdr->c_small.mask;
            ws_mask(ws);
            return WS_OK;
        }
        l = mlen + sizeof(wss_small_t);
        if (buf_len < l) return WS_WAIT;
        if (buf_len > l) return WS_TOOBIG;
        ws->ptr = (uint8_t*)buf + sizeof(wss_small_t);
        ws->mask = NULL;
        return WS_OK;
    } else
    if (mlen < 65535) {
        ws->type = WS_BIG;
        if (is_mask) {
            ws->len = ws->hdr->c_big.len = htobe16(ws->hdr->c_big.len);
            l = ws->len + sizeof(wsc_big_t);
            if (buf_len < l) return WS_WAIT;
            if (buf_len > l) return WS_TOOBIG;
            ws->ptr = (uint8_t*)buf + sizeof(wsc_big_t);
            ws->mask = ws->hdr->c_big.mask;
            ws_mask(ws);
            return WS_OK;
        }
        ws->len = ws->hdr->s_big.len = htobe16(ws->hdr->s_big.len);
        l = ws->len + sizeof(wss_big_t);
        if (buf_len < l) return WS_WAIT;
        if (buf_len < l) return WS_TOOBIG;
        ws->ptr = (uint8_t*)buf + sizeof(wss_big_t);
        ws->mask = NULL;
        return WS_OK;
    }
    ws->type = WS_HUGE;
    
    if (is_mask) {
        ws->len = ws->hdr->c_huge.len = htobe64(ws->hdr->c_huge.len);
        l = ws->len + sizeof(wsc_huge_t);
        if (buf_len < l) return WS_WAIT;
        if (buf_len > l) return WS_TOOBIG;
        ws->ptr = (uint8_t*)buf + sizeof(wsc_huge_t);
        ws->mask = ws->hdr->c_huge.mask;
        ws_mask(ws);
        return WS_OK;
    }
    ws->len = ws->hdr->s_huge.len = htobe64(ws->hdr->s_huge.len);
    l = ws->len + sizeof(wss_huge_t);
    if (buf_len < l) return WS_WAIT;
    if (buf_len > l) return WS_TOOBIG;
    ws->ptr = (uint8_t*)buf + sizeof(wss_huge_t);
    ws->mask = NULL;
    return WS_OK;
}

void ws_mask (ws_t *ws) {
    if (!ws->ptr || !ws->len || !ws->mask)
        return;
    for (size_t i = 0; i < ws->len; ++i)
        ws->ptr[i] = ws->ptr[i] ^ ws->mask[i % 4];
}

uint8_t bytes [sizeof(uint8_t)];

__attribute__ ((constructor))
static void ws_init () {
    for (int i = 0; i < sizeof(uint8_t); ++i)
        bytes[i] = i;
}

static void ws_set_header_small (strbuf_t *buf, ws_t *ws, unsigned int flags) {
    if ((flags & WS_MASK)) {
        uint8_t len = buf->len - sizeof(wsc_small_t);
        ws->hdr = (ws_hdr_t*)buf->ptr;
        ws->hdr->h.mlen = 0x80 | len;
        ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_small_t);
        ws->len = buf->len - sizeof(wsc_small_t);
        ws->mask = ws->hdr->c_small.mask;
        for (int i = 0; i < 4; ++i)
            ws->mask[i] = bytes[rand() % sizeof(uint8_t)];
    } else {
        uint8_t len = buf->len - sizeof(wss_small_t);
        ws->hdr = (ws_hdr_t*)buf->ptr;
        ws->hdr->h.mlen = len;
        ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_small_t);
        ws->len = buf->len - sizeof(wss_small_t);
        ws->mask = 0;
    }
}

static void ws_set_header_big (strbuf_t *buf, ws_t *ws, unsigned int flags) {
    if ((flags & WS_MASK)) {
        uint16_t len = buf->len - sizeof(wsc_big_t);
        ws->hdr = (ws_hdr_t*)buf->ptr;
        ws->hdr->h.mlen = 0x80 | 0x7e;
        ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_big_t);
        ws->len = buf->len - sizeof(wsc_big_t);
        ws->mask = ws->hdr->c_big.mask;
        ws->hdr->c_big.len = be16toh(len);
        for (int i = 0; i < 4; ++i)
            ws->mask[i] = bytes[rand() % sizeof(uint8_t)];
    } else {
        uint16_t len = buf->len - sizeof(wss_big_t);
        ws->hdr = (ws_hdr_t*)buf->ptr;
        ws->hdr->h.mlen = 0x7e;
        ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_big_t);
        ws->len = buf->len - sizeof(wss_big_t);
        ws->mask = 0;
        ws->hdr->s_big.len = be16toh(len);
    }
}

static void ws_set_header_huge (strbuf_t *buf, ws_t *ws, unsigned int flags) {
    if ((flags & WS_MASK)) {
        uint64_t len = buf->len - sizeof(wsc_huge_t);
        ws->hdr = (ws_hdr_t*)buf->ptr;
        ws->hdr->h.mlen = 0x80 | 0x7f;
        ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_huge_t);
        ws->len = buf->len - sizeof(wsc_huge_t);
        ws->mask = ws->hdr->c_huge.mask;
        ws->hdr->c_huge.len = be64toh(len);
        for (int i = 0; i < 4; ++i)
            ws->mask[i] = bytes[rand() % sizeof(uint8_t)];
    } else {
        uint64_t len = buf->len - sizeof(wss_huge_t);
        ws->hdr = (ws_hdr_t*)buf->ptr;
        ws->hdr->h.mlen = 0x7f;
        ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_huge_t);
        ws->len = buf->len - sizeof(wss_huge_t);
        ws->mask = 0;
        ws->hdr->s_huge.len = be64toh(len);
    }
}

void ws_set_header (strbuf_t *buf, ws_type_t type, unsigned int flags, uint8_t opcode) {
    ws_t ws;
    switch (type) {
        case WS_SMALL:
            ws_set_header_small(buf, &ws, flags);
            break;
        case WS_BIG:
            ws_set_header_big(buf, &ws, flags);
            break;
        case WS_HUGE:
            ws_set_header_huge(buf, &ws, flags);
            break;
        default:
            if (buf->len < 126)
                ws_set_header_small(buf, &ws, flags);
            else
            if (buf->len < 65535)
                ws_set_header_big(buf, &ws, flags);
            else
                ws_set_header_huge(buf, &ws, flags);
    }
    ws.hdr->h.b0 = opcode;
    if ((flags & WS_FIN))
        ws.hdr->h.b0 |= WS_FIN;
}

int ws_create (ws_type_t type, ws_t *ws, strbuf_t *buf, uint8_t op, uint8_t flags, int is_mask) {
    memset(ws, 0, sizeof(ws_t));
    switch (type) {
        case WS_SMALL:
            if (is_mask) {
                if (-1 == strbufsize(buf, sizeof(wsc_small_t), 0))
                    return WS_ERROR;
                buf->len = sizeof(wsc_small_t);
                ws->hdr = (ws_hdr_t*)buf->ptr;
                ws->hdr->h.mlen = 0x80 | 0;
                ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_small_t);
                ws->mask = ws->hdr->c_small.mask;
                for (int i = 0; i < 4; ++i)
                    ws->mask[i] = bytes[rand() % sizeof(uint8_t)];
            } else {
                if (-1 == strbufsize(buf, sizeof(wss_small_t), 0))
                    return WS_ERROR;
                buf->len = sizeof(wss_small_t);
                ws->hdr = (ws_hdr_t*)buf->ptr;
                ws->hdr->h.mlen = 0;
                ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_small_t);
                ws->mask = 0;
            }
            break;
        case WS_BIG:
            if (is_mask) {
                if (-1 == strbufsize(buf, sizeof(wsc_big_t), 0))
                    return WS_ERROR;
                buf->len = sizeof(wsc_big_t);
                ws->hdr = (ws_hdr_t*)buf->ptr;
                ws->hdr->h.mlen = 0x80 | 0x7e;
                ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_big_t);
                ws->mask = ws->hdr->c_big.mask;
                ws->hdr->c_big.len = 0;
                for (int i = 0; i < 4; ++i)
                    ws->mask[i] = bytes[rand() % sizeof(uint8_t)];
            } else {
                if (-1 == strbufsize(buf, sizeof(wss_big_t), 0))
                    return WS_ERROR;
                buf->len = sizeof(wss_big_t);
                ws->hdr = (ws_hdr_t*)buf->ptr;
                ws->hdr->h.mlen = 0x7e;
                ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_big_t);
                ws->mask = 0;
                ws->hdr->s_big.len = 0;
            }
            break;
        case WS_HUGE:
            if (is_mask) {
                if (-1 == strbufsize(buf, sizeof(wsc_huge_t), 0))
                    return WS_ERROR;
                buf->len = sizeof(wsc_huge_t);
                ws->hdr = (ws_hdr_t*)buf->ptr;
                ws->hdr->h.mlen = 0x80 | 0x7f;
                ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_huge_t);
                ws->mask = ws->hdr->c_huge.mask;
                ws->hdr->c_huge.len = 0;
                for (int i = 0; i < 4; ++i)
                    ws->mask[i] = bytes[rand() % sizeof(uint8_t)];
            } else {
                if (-1 == strbufsize(buf, sizeof(wss_huge_t), 0))
                    return WS_ERROR;
                buf->len = sizeof(wss_huge_t);
                ws->hdr = (ws_hdr_t*)buf->ptr;
                ws->hdr->h.mlen = 0x7f;
                ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_huge_t);
                ws->mask = 0;
                ws->hdr->s_huge.len = 0;
            }
            break;
        default:
            return WS_ERROR;
    }
    ws->type = type;
    ws->hdr->h.b0 = flags & (op >> 4);
    ws->len = 0;
    return WS_OK;
}

int ws_add (ws_t *ws, strbuf_t *buf, const char *src, size_t src_len) {
    if (-1 == strbufadd(buf, src, src_len))
        return WS_ERROR;
    switch (ws->type) {
        case WS_SMALL:
            if (ws->mask) {
                size_t nl = WS_BODY_LEN(ws) + src_len;
                if (nl < 126) {
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_small_t);
                        ws->mask = ws->hdr->c_small.mask;
                    }
                    ws->len = ws->hdr->h.mlen = 0x80 | nl;
                } else
                if (nl < 65535) {
                    if (-1 == strbufsize(buf, buf->len + sizeof(uint16_t), 0))
                        return WS_ERROR;
                    memmove(buf->ptr + sizeof(wsh_t) + sizeof(uint16_t), buf->ptr + sizeof(wsh_t), buf->len);
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->hdr->h.mlen = 0x80 | 0x7e;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_big_t);
                        ws->mask = ws->hdr->c_big.mask;
                    }
                    ws->len = ws->hdr->c_big.len = nl;
                } else {
                    if (-1 == strbufsize(buf, buf->len + sizeof(uint64_t), 0))
                        return WS_ERROR;
                    memmove(buf->ptr + sizeof(wsh_t) + sizeof(uint64_t), buf->ptr + sizeof(wsh_t), buf->len);
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->hdr->h.mlen = 0x80 | 0x7f;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_huge_t);
                        ws->mask = ws->hdr->c_huge.mask;
                    }
                    ws->len = ws->hdr->c_huge.len = nl;
                }
            } else {
                size_t nl = WS_BODY_LEN(ws);
                if (nl < 126) {
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_small_t);
                    }
                    ws->len = ws->hdr->h.mlen = nl;
                } else
                if (nl < 65535) {
                    if (-1 == strbufsize(buf, buf->len + sizeof(uint16_t), 0))
                        return -1;
                    memmove(buf->ptr + sizeof(wsh_t) + sizeof(uint16_t), buf->ptr + sizeof(wsh_t), buf->len);
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->hdr->h.mlen = 0x80 | 0x7e;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_big_t);
                    }
                    ws->len = ws->hdr->s_big.len = nl;
                } else {
                    if (-1 == strbufsize(buf, buf->len + sizeof(uint64_t), 0))
                        return WS_ERROR;
                    memmove(buf->ptr + sizeof(wsh_t) + sizeof(uint64_t), buf->ptr + sizeof(wsh_t), buf->len);
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->hdr->h.mlen = 0x7f;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_huge_t);
                    }
                    ws->len = ws->hdr->s_huge.len = nl;
                }
            }
            break;
        case WS_BIG:
            if (ws->mask) {
                size_t nl = ws->hdr->c_big.len + src_len;
                if (src_len < 65535) {
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_big_t);
                        ws->mask = ws->hdr->c_big.mask;
                    }
                    ws->len = ws->hdr->c_big.len = nl;
                } else {
                    if (-1 == strbufsize(buf, buf->len + (sizeof(uint64_t) - sizeof(uint16_t)), 0))
                        return WS_ERROR;
                    memmove(buf->ptr + sizeof(wsh_t) + sizeof(uint64_t), buf->ptr + sizeof(wsh_t) + sizeof(uint16_t), buf->len);
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->hdr->h.mlen = 0x80 | 0x7f;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_huge_t);
                        ws->mask = ws->hdr->c_huge.mask;
                    }
                    ws->len = ws->hdr->c_huge.len = nl;
                }
            } else {
                size_t nl = ws->hdr->s_big.len + src_len;
                if (nl < 65535) {
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_big_t);
                    }
                    ws->hdr->s_big.len = nl;
                } else {
                    if (-1 == strbufsize(buf, buf->len + (sizeof(uint64_t) - sizeof(uint16_t)), 0))
                        return WS_ERROR;
                    if (ERANGE == errno) {
                        ws->hdr = (ws_hdr_t*)buf->ptr;
                        ws->hdr->h.mlen = 0x7f;
                        ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_huge_t);
                    }
                    ws->len = ws->hdr->s_huge.len = nl;
                }
            }
            break;
        case WS_HUGE:
            if (ws->mask) {
                size_t nl = ws->hdr->c_huge.len + src_len;
                if (ERANGE == errno) {
                    ws->hdr = (ws_hdr_t*)buf->ptr;
                    ws->ptr = (uint8_t*)ws->hdr + sizeof(wsc_huge_t);
                }
                ws->len = ws->hdr->c_huge.len = nl;
            } else {
                size_t nl = ws->hdr->c_huge.len + src_len;
                if (ERANGE == errno) {
                    ws->hdr = (ws_hdr_t*)buf->ptr;
                    ws->ptr = (uint8_t*)ws->hdr + sizeof(wss_huge_t);
                }
                ws->len = ws->hdr->s_huge.len = nl;
            }
            break;
        default:
            return WS_ERROR;
    }
    return WS_OK;
}
