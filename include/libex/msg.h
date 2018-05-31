#ifndef __LIBEX_MSG_H__
#define __LIBEX_MSG_H__

#include "str.h"
#include "list.h"

#define MSG_INSERTED 0
#define MSG_NOT_INSERTED 1
#define MSG_ERROR -1

#define MSG_INIT { .len = 0, .bufsize = 0, .chunk_size = 0, .ptr = NULL, .pc = NULL, .method = 0, .cookie = { .ptr = NULL, .len = 0 }, .code = 0 }
typedef struct {
    uint32_t len;
    uint32_t bufsize;
    uint32_t chunk_size;
    char *ptr;
    char *pc;
    uint32_t method;
    strptr_t cookie;
    uint32_t code;
} msgbuf_t;

typedef int (*msg_item_h) (msgbuf_t*, void*, void*);
typedef void *(*msg_allocator_h) (size_t);
typedef void (*msg_deallocator_h) (void*);

extern msg_allocator_h msg_alloc;
extern msg_deallocator_h msg_free;

int msg_create_request (msgbuf_t *msg, uint32_t method, const char *cookie, size_t cookie_len, uint32_t len, uint32_t chunk_size);
int msg_create_response (msgbuf_t *msg, int code, uint32_t len, uint32_t chunk_size);
int msg_setstr (msgbuf_t *msg, const char *src, size_t src_len);
int msg_setbuf (msgbuf_t *msg, void *src, uint32_t src_len);
static inline int msg_seti (msgbuf_t *msg, int val) { return msg_setbuf(msg, &val, sizeof(int)); };
static inline int msg_seti32 (msgbuf_t *msg, int32_t val) { return msg_setbuf(msg, &val, sizeof(int32_t)); };
static inline int msg_setui32 (msgbuf_t *msg, uint32_t val) { return msg_setbuf(msg, &val, sizeof(uint32_t)); };
static inline int msg_setd (msgbuf_t *msg, double val) { return msg_setbuf(msg, &val, sizeof(double)); };
int msg_setlist (msgbuf_t *msg, list_t *lst, msg_item_h fn, void *userdata);
int msg_load_request (msgbuf_t *msg, char *buf, size_t buflen);
int msg_load_response (msgbuf_t *msg, char *buf, size_t buflen);
int msg_geti (msgbuf_t *msg, int *val);
int msg_geti32 (msgbuf_t *msg, int32_t *val);
int msg_getui32 (msgbuf_t *msg, uint32_t *val);
int msg_getd (msgbuf_t *msg, double *val);
int msg_getstr (msgbuf_t *buf, strptr_t *str);
int msg_enum (msgbuf_t *msg, msg_item_h fn, void *userdata);
static inline void msg_clear (msgbuf_t *msg) { if (msg->ptr) msg_free(msg->ptr); msg->ptr = NULL; };

int msg_error (msgbuf_t *msg, int code, const char *str, size_t len);
static inline int msg_ok(msgbuf_t *msg) { return msg_create_response(msg, 0, 8, 8); };

#endif // __LIBEX_MSG_H__
