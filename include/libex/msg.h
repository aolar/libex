/**
 * @file msg.h
 * @brief libex messages functions for binary protocol
 */
#ifndef __LIBEX_MSG_H__
#define __LIBEX_MSG_H__

#include "str.h"
#include "list.h"

/** message is inserted */
#define MSG_INSERTED 0
/** message not insertede */
#define MSG_NOT_INSERTED 1
/** message error */
#define MSG_ERROR -1
/** message ok */
#define MSG_OK 0

/** message initializer */
#define MSG_INIT { .len = 0, .bufsize = 0, .chunk_size = 0, .ptr = NULL, .pc = NULL, .method = 0, .cookie = { .ptr = NULL, .len = 0 }, .code = 0 }
/** @brief message structure */
typedef struct {
    /** message length */
    uint32_t len;
    /** message buffer size */
    uint32_t bufsize;
    /** chunk size */
    uint32_t chunk_size;
    /** pointer to message */
    char *ptr;
    /** current pointer for reading/writing message elements */
    char *pc;
    /** method identifier */
    uint32_t method;
    /** cookish */
    strptr_t cookie;
    /** error code */
    uint32_t code;
    /** error message for \ code, nothing if success */
    strptr_t errmsg;
} msgbuf_t;

/** callback for list appending */
typedef int (*msg_item_h) (msgbuf_t*, void*, void*);
/** allocatore, required in nanomsg */
typedef void *(*msg_allocator_h) (size_t);
/** deallocator, requirede for nanomsg */
typedef void (*msg_deallocator_h) (void*)
/** reallocator, required for nano msg */;
typedef void *(*msg_reallocator_h) (void*, size_t);

/** variable for allocator */
extern msg_allocator_h msg_alloc;
/** variable for deaqllocator */
extern msg_deallocator_h msg_free;
/** variable fro reallocator */
extern msg_reallocator_h msg_realloc;

/** @brief create request 
 * @param msg message struct
 * @param method method identifier
 * @param cookie
 * @param cookie_len
 * @param len start legtnh of message
 * @param chunk_size
 * @retval 0 if success, -1 if error
 */
int msg_create_request (msgbuf_t *msg, uint32_t method, const char *cookie, size_t cookie_len, uint32_t len, uint32_t chunk_size);

/** @brief create response
 * @param msg message struct
 * @param code error code
 * @param len start legtnh of message
 * @param chunk_size
 * @retval 0 if success, -1 if error
 */
int msg_create_response (msgbuf_t *msg, int code, uint32_t len, uint32_t chunk_size);

/** @brief append string into message
 * @param msg
 * @src string
 * @src_len string length
 * @retval 0 if success, -1 if error
 */
int msg_setstr (msgbuf_t *msg, const char *src, size_t src_len);

/** @brief append buffer into message
 * @param msg
 * @src string
 * @src_len string length
 * @retval 0 if success, -1 if error
 */
int msg_setbuf (msgbuf_t *msg, void *src, uint32_t src_len);

/** @brief append integer into message
 * @param msg
 * @src string
 * @src_len string length
 * @retval 0 if success, -1 if error
 */
static inline int msg_seti (msgbuf_t *msg, int val) { return msg_setbuf(msg, &val, sizeof(int)); };

/** @brief append integer 32 into message
 * @param msg
 * @src string
 * @src_len string length
 * @retval 0 if success, -1 if error
 */
static inline int msg_seti32 (msgbuf_t *msg, int32_t val) { return msg_setbuf(msg, &val, sizeof(int32_t)); };

/** @brief append unsigned integer 32 into message
 * @param msg
 * @src string
 * @src_len string length
 * @retval 0 if success, -1 if error
 */
static inline int msg_setui32 (msgbuf_t *msg, uint32_t val) { return msg_setbuf(msg, &val, sizeof(uint32_t)); };

/** @brief append double into message
 * @param msg
 * @src string
 * @src_len string length
 * @retval 0 if success, -1 if error
 */
static inline int msg_setd (msgbuf_t *msg, double val) { return msg_setbuf(msg, &val, sizeof(double)); };

/** @brief append list into message
 * @param msg
 * @param lst
 * @param fn callback for inserting each elements into message,
 * function prototype is int (*msg_item_h) (msgbuf_t *msg, void *item_data, void *userdata)
 * @param userdata
 * @retval 0 if success, -1 if error
 */
int msg_setlist (msgbuf_t *msg, list_t *lst, msg_item_h fn, void *userdata);

/** @brief parse buffer as request
 * @param msg
 * @param buf
 * @param buflen
 * @retval 0 if success, -1 if error
 */
int msg_load_request (msgbuf_t *msg, char *buf, size_t buflen);

/** @brief parse buffer as response
 * @param msg
 * @param buf
 * @param buflen
 * @retval 0 if success, -1 if error
 */
int msg_load_response (msgbuf_t *msg, char *buf, size_t buflen);

/** @brief get data from message as integer
 * @param msg
 * @param val result
 * @retval 0 if success, -1 if error
 */
int msg_geti (msgbuf_t *msg, int *val);

/** @brief get data from message as 32 bit integer
 * @param msg
 * @param val result
 * @retval 0 if success, -1 if error
 */
int msg_geti32 (msgbuf_t *msg, int32_t *val);

/** @brief get data from message as 32 bit unsigned integer
 * @param msg
 * @param val result
 * @retval 0 if success, -1 if error
 */
int msg_getui32 (msgbuf_t *msg, uint32_t *val);

/** @brief get data from message as double
 * @param msg
 * @param val result
 * @retval 0 if success, -1 if error
 */
int msg_getd (msgbuf_t *msg, double *val);

/** @brief get data from message as string
 * @param msg
 * @param val result
 * @retval 0 if success, -1 if error
 */
int msg_getstr (msgbuf_t *buf, strptr_t *str);

/** @brief enumerate list in message
 * @param msg
 * @param fn
 * @param userdata
 * @retval 0 if success, -1 if error
 */
int msg_enum (msgbuf_t *msg, msg_item_h fn, void *userdata);

/** @brief free message buffer
 * @param msg
 */
static inline void msg_clear (msgbuf_t *msg) { if (msg->ptr) msg_free(msg->ptr); msg->ptr = NULL; };

/** @brief create message response as error result
 * @param msg
 * @param code code result
 * @param str error string
 * @param len error string length
 * @retval 0 if success, -1 if error
 */
int msg_error (msgbuf_t *msg, int code, const char *str, size_t len);

/** @brief create message response as successed result
 * @param msg
 */
static inline int msg_ok (msgbuf_t *msg) { return msg_create_response(msg, 0, 8, 8); };

/** @brief destroy message
 * @param msg
 */
static inline void msg_destroy (msgbuf_t *msg) { if (msg->ptr) { free(msg->ptr); memset(msg, 0, sizeof(msgbuf_t)); } };

#endif // __LIBEX_MSG_H__
