#ifndef __LIBEX_WS_H__
#define __LIBEX_WS_H__

#include <stdint.h>
#include <endian.h>
#include "http.h"

#define WS_SECKEY_LEN 24
#define WS_UUID_LEN 36
#define SHA1_LEN 20

#define WS_FIN  0b10000000
#define WS_RSV1 0b01000000
#define WS_RSV2 0b00100000
#define WS_RSV3 0b00010000

#define WS_ISFIN(x) (x->hdr->h.b0 & WS_FIN)

#define WS_FRAFMENT 0x0
#define WS_TEXT 0x1
#define WS_BIN  0x2
#define WS_CLOSE 0x8
#define WS_PING 0x9
#define WS_PONG 0xa
#define WS_OPCODE(x) x->hdr->h.b0 & 0x0f

#define WS_BODY_LEN(x) (x->hdr->h.mlen & 0x7f)
#define WS_ISMASK(x) (x->hdr->h.mlen & 0x80)
#define WS_HDRLEN sizeof(uint8_t) * 2
#define WS_MASKLEN sizeof(uint8_t) * 4

#define WS_WAIT 1
#define WS_OK 0
#define WS_ERROR -1
#define WS_TOOBIG -2

#define WS_MASK 0b00000001

typedef enum { WS_AUTO, WS_SMALL, WS_BIG, WS_HUGE } ws_type_t;

typedef struct {
    char sec_key [WS_SECKEY_LEN + WS_UUID_LEN + 1];
    char key [SHA1_LEN+1];
} ws_handshake_t;

int ws_handshake (http_request_t *req, char *buf, size_t buf_len, ws_handshake_t *wsh);
void ws_make_response (strbuf_t *buf, ws_handshake_t *wsh);

typedef struct {
    uint8_t b0;
    uint8_t mlen;
} wsh_t;

typedef struct {
    wsh_t h;
    uint8_t mask [4];
    uint8_t data [0];
} wsc_small_t;

typedef struct {
    wsh_t h;
    uint8_t data [0];
} wss_small_t;

typedef struct {
    wsh_t h;
    uint16_t len;
    uint8_t mask [4];
    uint8_t data [0];
} wsc_big_t;

typedef struct {
    wsh_t h;
    uint16_t len;
    uint8_t data [0];
} wss_big_t;

typedef struct {
    wsh_t h;
    uint64_t len;
    uint8_t mask [4];
    uint8_t data [0];
} wsc_huge_t;

typedef struct {
    wsh_t h;
    uint64_t len;
    uint8_t data [0];
} wss_huge_t;

typedef union {
    wsh_t h;
    wsc_small_t c_small;
    wss_small_t s_small;
    wsc_big_t c_big;
    wss_big_t s_big;
    wsc_huge_t c_huge;
    wss_huge_t s_huge;
    char *ptr;
} ws_hdr_t;

typedef struct {
    ws_hdr_t *hdr;
    ws_type_t type;
    uint8_t *ptr;
    size_t len;
    uint8_t *mask;
} ws_t;

int ws_parse (const char *buf, size_t buf_len, ws_t *ws);
void ws_mask (ws_t *ws);
int ws_create (ws_type_t type, ws_t *ws, strbuf_t *buf, uint8_t op, uint8_t flags, int is_mask);
int ws_add (ws_t *ws, strbuf_t *buf, const char *src, size_t src_len);
void ws_set_header (strbuf_t *buf, ws_type_t type, unsigned int flags, uint8_t opcode);

ssize_t ws_buflen (const uint8_t *buf, size_t buflen);

#endif // __LIBEX_WS_H__
