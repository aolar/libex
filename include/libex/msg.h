#ifndef __LIBEX_MSG_H__
#define __LIBEX_MSG_H__

#include "str.h"
#include "list.h"

#define MSG_INSERTED 0
#define MSG_NOT_INSERTED 1
#define MSG_ERROR -1

typedef struct {
    uint32_t len;
    uint32_t bufsize;
    uint32_t chunk_size;
    char *ptr;
    char *pc;
} msgbuf_t;

typedef int (*msg_item_h) (msgbuf_t*, void*, void*);

int msg_alloc (msgbuf_t *msg, uint32_t method, const char *cookie, size_t cookie_len, uint32_t chunk_size);
int msg_setstr (msgbuf_t *msg, const char *src, size_t src_len);
int msg_setbuf (msgbuf_t *msg, void *src, uint32_t src_len);
static inline int msg_seti (msgbuf_t *msg, int val) { return msg_setbuf(msg, &val, sizeof(int)); };
static inline int msg_seti32 (msgbuf_t *msg, int32_t val) { return msg_setbuf(msg, &val, sizeof(int32_t)); };
static inline int msg_setui32 (msgbuf_t *msg, uint32_t val) { return msg_setbuf(msg, &val, sizeof(uint32_t)); };
static inline int msg_setd (msgbuf_t *msg, double val) { return msg_setbuf(msg, &val, sizeof(double)); };
int msg_setlist (msgbuf_t *msg, list_t *lst, msg_item_h fn, void *userdata);
int msg_geti (msgbuf_t *msg, int *val);
int msg_geti32 (msgbuf_t *msg, int32_t *val);
int msg_getui32 (msgbuf_t *msg, uint32_t *val);
int msg_getd (msgbuf_t *msg, double *val);
int msg_getstr (msgbuf_t *buf, strptr_t *str);
int msg_enum (msgbuf_t *msg, msg_item_h fn, void *userdata);

#endif // __LIBEX_MSG_H__
