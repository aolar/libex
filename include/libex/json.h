#ifndef __LIBEX_JSON_H__
#define __LIBEX_JSON_H__

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include "str.h"
#include "list.h"
#include "tree.h"

#define JSON_ANY       -1
#define JSON_OBJECT     1
#define JSON_ARRAY      2
#define JSON_STRING     3
#define JSON_INTEGER    4
#define JSON_DOUBLE     5
#define JSON_TRUE       6
#define JSON_FALSE      7
#define JSON_NULL       8

typedef rbtree_t json_object_t;
typedef list_t json_array_t;
typedef struct json_item {
    strptr_t key;
    int type;
    union {
        strptr_t s;
        int64_t i;
        long double d;
        json_object_t *o;
        json_array_t *a;
    } data;
} json_item_t;

typedef struct {
    char *text;
    char *text_ptr;
    size_t text_len;
    int type;
    union {
        json_object_t *o;
        json_array_t *a;
    } data;
} json_t;

#define JSON_ISNAME(j,S) 0 == cmpstr(j->key.ptr, j->key.len, CONST_STR_LEN(S))

typedef int (*json_item_h) (json_item_t*, void*);
json_t *json_parse (const char *json_str);
json_t *json_parse_len (const char *json_str, size_t json_str_len);
json_item_t *json_find (json_object_t *jo, const char *key, size_t key_len, int type);
void json_enum_array (json_array_t *lst, json_item_h fn, void *userdata, int flags);
void json_enum_object (json_object_t *obj, json_item_h fn, void *userdata, int flags);
static inline char *json_str(json_item_t *j) { return strndup(j->data.s.ptr, j->data.s.len); }
void json_free (json_t *j);

#define JSON_NOT_INSERTED 0
#define JSON_INSERTED 1

typedef int (*json_list_item_h) (list_item_t*, strbuf_t*, void*);
typedef int (*json_tree_item_h) (tree_item_t*, strbuf_t*, void*);

#define JSON_NEXT 0
#define JSON_END 1
#define JSON_ITEM JSON_END
#define JSON_DOUBLE_PRECISION 6

void json_add_key (strbuf_t *buf, const char *key, size_t key_len);
void json_add_str (strbuf_t *buf, const char *key, size_t key_len, const char *val, size_t val_len, int is_end);
void json_add_escape_str (strbuf_t *buf, const char *key, size_t key_len, const char *val, size_t val_len, int is_end);
void json_add_int (strbuf_t *buf, const char *key, size_t key_len, int64_t val, int is_end);
void json_add_double_p (strbuf_t *buf, const char *key, size_t key_len, long double val, int prec, int is_end);
static inline void json_add_double (strbuf_t *buf, const char *key, size_t key_len, long double val, int is_end) { json_add_double_p(buf, key, key_len, val, JSON_DOUBLE_PRECISION, is_end); }
void json_add_null (strbuf_t *buf, const char *key, size_t key_len, int is_end);
void json_add_bool (strbuf_t *buf, const char *key, size_t key_len, int val, int is_end);
static inline void json_add_true (strbuf_t *buf, const char *key, size_t key_len, int is_end) { json_add_bool(buf, key, key_len, 1, is_end); }
static inline void json_add_false (strbuf_t *buf, const char *key, size_t key_len, int is_end) { json_add_bool(buf, key, key_len, 0, is_end); }
static inline void json_add_item_str (strbuf_t *buf, const char *val, size_t val_len) { json_add_str(buf, CONST_STR_NULL, val, val_len, JSON_ITEM); }
static inline void json_add_item_int (strbuf_t *buf, int64_t val) { json_add_int(buf, CONST_STR_NULL, val, JSON_ITEM); }
static inline void json_add_item_null (strbuf_t *buf) { json_add_null(buf, CONST_STR_NULL, JSON_ITEM); }
static inline void json_add_item_bool (strbuf_t *buf, int val) { json_add_bool(buf, CONST_STR_NULL, val, JSON_ITEM); }
static inline void json_add_item_true (strbuf_t *buf) { json_add_bool(buf, CONST_STR_NULL, 1, JSON_ITEM); }
static inline void json_add_item_false (strbuf_t *buf) { json_add_bool(buf, CONST_STR_NULL, 0, JSON_ITEM); }
void json_open_array (strbuf_t *buf, const char *key, size_t key_len);
void json_close_array (strbuf_t *buf, int is_end);
static inline void json_empty_array (strbuf_t *buf, const char *key, size_t key_len, int is_end) { json_open_array(buf, key, key_len); json_close_array(buf, is_end); };
void json_add_list (strbuf_t *buf, const char *key, size_t key_len, list_t *list, json_list_item_h on_item, void *userdata, int is_end);
void json_add_tree (strbuf_t *buf, const char *key, size_t key_len, rbtree_t *tree, json_tree_item_h on_item, void *userdata, int is_end);
void json_open_object (strbuf_t *buf, const char *key, size_t key_len);
void json_close_object (strbuf_t *buf, int is_end);
static inline void json_empty_object (strbuf_t *buf, const char *key, size_t key_len, int is_end) { json_open_object(buf, key, key_len); json_close_object(buf, is_end); };
static inline void json_begin_object (strbuf_t *buf) { json_open_object(buf, CONST_STR_NULL); }
static inline void json_end_object (strbuf_t *buf) { json_close_object(buf, JSON_END); }
static inline void json_begin_array (strbuf_t *buf) { json_open_array(buf, CONST_STR_NULL); }
static inline void json_end_array (strbuf_t *buf) { json_close_array(buf, JSON_END); }

#define JSONRPC_OK 0
#define JSONRPC_PARSE_ERROR -32700
#define JSONRPC_INVALID_REQUEST -32600
#define JSONRPC_METHOD_NOT_FOUND -32601
#define JSONRPC_INVALID_PARAMS -32602
#define JSONRPC_INTERNAL_ERROR -32603
#define JSONRPC_ERROR -32000

#define JSONRPC_ID_INT(X) X, -1
#define JSONRPC_ID_NULL 0, 0
#define JSONRPC_ID_STR(X) CONST_STR_LEN(X)

typedef enum {
    JSONRPC_VNONE,
    JSONRPC_V10,
    JSONRPC_V20
} jsonrpc_ver_t;

typedef struct {
    jsonrpc_ver_t ver;
    json_item_t *id;
    strptr_t method;
    json_item_t *params;
    json_item_t *result;
    int64_t error_code;
    strptr_t error_message;
    json_item_t *error_data;
    json_t *json;
} jsonrpc_t;

typedef struct {
    jsonrpc_t *jsonrpc;
    int errcode;
} jsonrpc_enum_t;

typedef json_t *(*json_parse_h) (const char*, size_t);

int jsonrpc_parse_request_intr (const char *json_str, size_t json_str_len, json_parse_h on_parse, jsonrpc_t *jsonrpc);
int jsonrpc_parse_response_intr (const char *json_str, size_t json_str_len, json_parse_h on_parse, jsonrpc_t *jsonrpc);

static inline int jsonrpc_parse_request_len (const char *json_str, size_t json_str_len, jsonrpc_t *jsonrpc) { return jsonrpc_parse_request_intr(json_str, json_str_len, json_parse_len, jsonrpc); };
static inline int jsonrpc_parse_request (const char *json_str, jsonrpc_t *jsonrpc) { return jsonrpc_parse_request_intr(json_str, strlen(json_str), json_parse_len, jsonrpc); };
static inline int jsonrpc_parse_response_len (const char *json_str, size_t json_str_len, jsonrpc_t *jsonrpc) { return jsonrpc_parse_response_intr(json_str, json_str_len, json_parse_len, jsonrpc); };
static inline int jsonrpc_parse_response (const char *json_str, jsonrpc_t *jsonrpc) { return jsonrpc_parse_response_intr(json_str, strlen(json_str), json_parse_len, jsonrpc); };

typedef int (*jsonrpc_h) (strbuf_t*, void *userdata);
void jsonrpc_setver (jsonrpc_ver_t ver);

extern char json_prefix;
extern size_t json_prefix_len;

void jsonrpc_prepare (strbuf_t *buf);
int jsonrpc_request (strbuf_t *buf, const char *method, size_t method_len, intptr_t id, int id_len, jsonrpc_h on_params, void *userdata);
int jsonrpc_response (strbuf_t *buf, jsonrpc_h on_result, void *userdata, intptr_t id, int id_len);
void jsonrpc_response_begin (strbuf_t *buf, intptr_t id, int id_len);
void jsonrpc_response_end (strbuf_t *buf);
static inline void jsonrpc_response_ok (strbuf_t *buf, intptr_t id, int id_len) {
    jsonrpc_response(buf, ({
        int fn (strbuf_t *buf, void *dummy) {
            json_add_true(buf, CONST_STR_NULL, JSON_END);
            return 0;
        } fn;
    }), NULL, id, id_len);
}
int jsonrpc_error (strbuf_t *buf, int code, const char *message, size_t message_len, intptr_t id, int id_len);
int jsonrpc_stderror (strbuf_t *buf, int code, intptr_t id, int id_len);

typedef void (*jsonrpc_method_h) (strbuf_t *buf, json_item_t **params, size_t params_len, intptr_t id, int id_len, void*);

typedef struct {
    int id;
    strptr_t method;
    jsonrpc_method_h handle;
    int param_lens;
    int *param_types;
} jsonrpc_method_t;

int jsonrpc_execute (strbuf_t *buf, size_t off, jsonrpc_method_t *methods, void *userdata);

#endif // __LIBEX_JSON_H__
