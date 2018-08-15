/**
 * @file json.h
 * @brief json, jsonrpc functions
 */
#ifndef __LIBEX_JSON_H__
#define __LIBEX_JSON_H__

#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include "str.h"
#include "list.h"
#include "tree.h"

/**
 * any json type
 */
#define JSON_ANY       -1
/**
 * object type
 */
#define JSON_OBJECT     1
/**
 * array type
 */
#define JSON_ARRAY      2
/**
 * string type
 */
#define JSON_STRING     3
/**
 * integer type
 */
#define JSON_INTEGER    4
/**
 * double type
 */
#define JSON_DOUBLE     5
/**
 * boolean type true value
 */
#define JSON_TRUE       6
/**
 * boolean type false value
 */
#define JSON_FALSE      7
/**
 * null value
 */
#define JSON_NULL       8

/**
 * @brief object type kept as rb tree
 */
typedef rbtree_t json_object_t;
/**
 * @brief array type kept as list
 */
typedef list_t json_array_t;
/**
 * @brief json item structure
 */
typedef struct json_item {
    /**
     * key name
     */
    strptr_t key;
    /**
     * item type
     */
    int type;
    /**
     * data
     */
    union {
        /**
         * string data
         */
        strptr_t s;
        /**
         * integer data
         */
        int64_t i;
        /**
         * double data
         */
        long double d;
        /**
         * object data
         */
        json_object_t *o;
        /**
         * array data
         */
        json_array_t *a;
    } data;
} json_item_t;

/**
 * @brief json structure
 */
typedef struct {
    /**
     * json as string
     */
    char *text;
    /**
     * current pointer in \b text, used for parsing
     */
    char *text_ptr;
    /**
     * length of \b text
     */
    size_t text_len;
    /**
     * root item type
     */
    int type;
    /**
     * root item
     */
    union {
        /**
         * root as object
         */
        json_object_t *o;
        /**
         * root as array
         */
        json_array_t *a;
    } data;
} json_t;

/**
 * tests name of json item
 */
#define JSON_ISNAME(j,S) 0 == cmpstr(j->key.ptr, j->key.len, CONST_STR_LEN(S))

/**
 * callback uses for enumeration of json
 */
typedef int (*json_item_h) (json_item_t*, void*);

/**
 * @brief parse json and create json structure
 * @param json_str json string
 * @return json object
 */
json_t *json_parse (const char *json_str);

/**
 * @brief parse json object and create json structure
 * @param json_str json string
 * @param json_str_len json string length
 * @return json object
 */
json_t *json_parse_len (const char *json_str, size_t json_str_len);

/**
 * @brief find json item in json object by key name and item type
 * @param jo json object
 * @param key key name
 * @param key_len key name length
 * @param type
 * <ul>
 *  <li> JSON_OBJECT
 *  <li> JSON_ARRAY
 *  <li> JSON_STRING
 *  <li> JSON_INTEGER
 *  <li> JSON_DOUBLE
 *  <li> JSON_TRUE
 *  <li> JSON_FALSE
 *  <li> JSON_NULL
 *  <li> JSON_ANY
 * </ul>
 */
json_item_t *json_find (json_object_t *jo, const char *key, size_t key_len, int type);

/**
 * @brief calls callback function for each json item in json array
 * @param lst array
 * @param fn callback function
 * @param userdata any data, uses as parameter in callback function
 * @param flags
 * <ul>
 *  <li> 0
 *  <li> ENUM_STOP_IF_BREAK if callback function returns ENUM_BREAK then enumeration will be stops
 * </ul>
 */
void json_enum_array (json_array_t *lst, json_item_h fn, void *userdata, int flags);

/**
 * @brief calls callback function for each json item in json object
 * @param obj object
 * @param fn callback function
 * @param userdata any data, uses as parameter in callback function
 * @param flags
 * <ul>
 *  <li> 0
 *  <li> ENUM_STOP_IF_BREAK if callback function returns ENUM_BREAK then enumeration will be stops
 * </ul>
 */
void json_enum_object (json_object_t *obj, json_item_h fn, void *userdata, int flags);

/**
 * @brief returns string from json string item
 */
static inline char *json_str(json_item_t *j) { return strndup(j->data.s.ptr, j->data.s.len); }

/**
 * @brief free json
 * @param j
 */
void json_free (json_t *j);

/**
 * json item not inserted
 */
#define JSON_NOT_INSERTED 0
/**
 * json item inserted
 */
#define JSON_INSERTED 1

/**
 * @brief callback function for inserting list item into json
 */
typedef int (*json_list_item_h) (list_item_t*, strbuf_t*, void*);
/**
 * @brief callback function for inserting tree item into json
 */
typedef int (*json_tree_item_h) (tree_item_t*, strbuf_t*, void*);

/**
 * flag for inserting the current item
 */
#define JSON_NEXT 0
/**
 * flags for inserting the end item
 */
#define JSON_END 1
/**
 * flags for inserting current element into array or object
 */
#define JSON_ITEM JSON_END

/**
 * default double precision
 */
#define JSON_DOUBLE_PRECISION 6

/**
 * @brief add key into json string buffer
 * @param buf string buffer
 * @param key key name
 * @param key_len key name length
 */
void json_add_key (strbuf_t *buf, const char *key, size_t key_len);

/**
 * @brief add string value into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key_len name length
 * @param val string value
 * @param val_len string value length
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_add_str (strbuf_t *buf, const char *key, size_t key_len, const char *val, size_t val_len, int is_end);

/**
 * @brief escape string and add into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key_len name length
 * @param val string value
 * @param val_len string value length
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_add_escape_str (strbuf_t *buf, const char *key, size_t key_len, const char *val, size_t val_len, int is_end);

/**
 * @brief add integer value into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key_len name length
 * @param val integer value
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_add_int (strbuf_t *buf, const char *key, size_t key_len, int64_t val, int is_end);

/**
 * @brief add double value into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key_len name length
 * @param val double value
 * @param prec precision of double value
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_add_double_p (strbuf_t *buf, const char *key, size_t key_len, long double val, int prec, int is_end);

/**
 * @brief add double value with default precision into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key name length
 * @param val double value
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
static inline void json_add_double (strbuf_t *buf, const char *key, size_t key_len, long double val, int is_end) { json_add_double_p(buf, key, key_len, val, JSON_DOUBLE_PRECISION, is_end); }

/**
 * @brief add null value into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key_len name length
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_add_null (strbuf_t *buf, const char *key, size_t key_len, int is_end);

/**
 * @brief add boolean value into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key_len name length
 * @param val boolean value, can be 1 or 0
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_add_bool (strbuf_t *buf, const char *key, size_t key_len, int val, int is_end);

/**
 * @brief add true value into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key name length
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
static inline void json_add_true (strbuf_t *buf, const char *key, size_t key_len, int is_end) { json_add_bool(buf, key, key_len, 1, is_end); }

/**
 * @brief add false value into json string buffer
 * @param buf json string buffer
 * @param key key name
 * @param key name length
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
static inline void json_add_false (strbuf_t *buf, const char *key, size_t key_len, int is_end) { json_add_bool(buf, key, key_len, 0, is_end); }

/**
 * @brief add string value into array of json string buffer
 * @param buf string buffer
 * @param val string value
 * @param val_len string value length
 */
static inline void json_add_item_str (strbuf_t *buf, const char *val, size_t val_len) { json_add_str(buf, CONST_STR_NULL, val, val_len, JSON_ITEM); }

/**
 * @brief add integer value into array of json string buffer
 * @param buf string buffer
 * @param val integer value
 */
static inline void json_add_item_int (strbuf_t *buf, int64_t val) { json_add_int(buf, CONST_STR_NULL, val, JSON_ITEM); }

/**
 * @brief add null value into array of json string buffer
 * @param buf string buffer
 */
static inline void json_add_item_null (strbuf_t *buf) { json_add_null(buf, CONST_STR_NULL, JSON_ITEM); }

/**
 * @brief add boolean value into array of json string buffer
 * @param buf string buffer
 * @param val boolean value, can be 0 or 1
 * @param val_len string value length
 */
static inline void json_add_item_bool (strbuf_t *buf, int val) { json_add_bool(buf, CONST_STR_NULL, val, JSON_ITEM); }

/**
 * @brief add true value into array of json string buffer
 * @param buf string buffer
 * @param val true value
 */
static inline void json_add_item_true (strbuf_t *buf) { json_add_bool(buf, CONST_STR_NULL, 1, JSON_ITEM); }

/**
 * @brief add false value into array of json string buffer
 * @param buf string buffer
 * @param val false value
 */
static inline void json_add_item_false (strbuf_t *buf) { json_add_bool(buf, CONST_STR_NULL, 0, JSON_ITEM); }

/**
 * @brief open array in string buffer
 * @param buf string buffer
 * @param key key name
 * @param key_len key name length
 */
void json_open_array (strbuf_t *buf, const char *key, size_t key_len);

/**
 * @brief close array in string buffer
 * @param buf string buffer
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_close_array (strbuf_t *buf, int is_end);

/**
 * @brief create empty array in string buffer
 * @param buf string buffer
 * @param key key name
 * @param key_len key name length
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
static inline void json_empty_array (strbuf_t *buf, const char *key, size_t key_len, int is_end) { json_open_array(buf, key, key_len); json_close_array(buf, is_end); };

/**
 * @brief add list into json string buffer
 * @param buf string buffer
 * @param key key name
 * @param key_len key name length
 * @param list
 * @param on_item callback function for each list element for inserting
 * @param userdata
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_add_list (strbuf_t *buf, const char *key, size_t key_len, list_t *list, json_list_item_h on_item, void *userdata, int is_end);

/**
 * @brief add tree into json string buffer
 * @param buf string buffer
 * @param key key name
 * @param key_len key name length
 * @param tree
 * @param on_item callback function for each tree element for inserting
 * @param userdata
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_add_tree (strbuf_t *buf, const char *key, size_t key_len, rbtree_t *tree, json_tree_item_h on_item, void *userdata, int is_end);

/**
 * @brief open object in string buffer
 * @param buf string buffer
 * @param key key name
 * @param key_len key name length
 */
void json_open_object (strbuf_t *buf, const char *key, size_t key_len);

/**
 * @brief close object in json string buffer
 * @param buf string buffer
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
void json_close_object (strbuf_t *buf, int is_end);

/**
 * @brief create empty  object in string buffer
 * @param buf string buffer
 * @param key key name
 * @param key_len key name length
 * @param is_end
 * <ul>
 *  <li> JSON_NEXT
 *  <li> JSON_END
 * </ul>
 */
static inline void json_empty_object (strbuf_t *buf, const char *key, size_t key_len, int is_end) { json_open_object(buf, key, key_len); json_close_object(buf, is_end); };

/**
 * @brief begin object in json string buffer
 * @param buf string buffer
 */
static inline void json_begin_object (strbuf_t *buf) { json_open_object(buf, CONST_STR_NULL); }

/**
 * @brief close object in json string buffer
 * @param buf string buffer
 */
static inline void json_end_object (strbuf_t *buf) { json_close_object(buf, JSON_END); }

/**
 * @brief begin array in json string buffer
 * @param buf string buffer
 */
static inline void json_begin_array (strbuf_t *buf) { json_open_array(buf, CONST_STR_NULL); }

/**
 * @brief close array in json string buffer
 * @param buf string buffer
 */
static inline void json_end_array (strbuf_t *buf) { json_close_array(buf, JSON_END); }

/**
 * json rpc ok
 */
#define JSONRPC_OK 0
/**
 * Invalid JSON was received by the server.
 * An error occurred on the server while parsing the JSON text.
 */
#define JSONRPC_PARSE_ERROR -32700
/**
 * The JSON sent is not a valid Request object.
 */
#define JSONRPC_INVALID_REQUEST -32600
/**
 * The method does not exist / is not available.
 */
#define JSONRPC_METHOD_NOT_FOUND -32601
/**
 * Invalid method parameter(s).
 */
#define JSONRPC_INVALID_PARAMS -32602
/**
 * Internal JSON-RPC error.
 */
#define JSONRPC_INTERNAL_ERROR -32603
/**
 * Reserved for implementation-defined server-errors.
 */
#define JSONRPC_ERROR -32000

/**
 * integer value json rpc identifier
 */
#define JSONRPC_ID_INT(X) X, -1
/**
 * null value json rpc identifier
 */
#define JSONRPC_ID_NULL 0, 0
/**
 * string value json rpc identifier
 */
#define JSONRPC_ID_STR(X) CONST_STR_LEN(X)

/**
 * json rpc versions
 */
typedef enum {
    /**
     * unknown version
     */
    JSONRPC_VNONE,
    /**
     * 1.0 version
     */
    JSONRPC_V10,
    /**
     * 2.0 version
     */
    JSONRPC_V20
} jsonrpc_ver_t;

/**
 * @brief json rp structure
 */
typedef struct {
    /**
     * version
     */
    jsonrpc_ver_t ver;
    /**
     * json item identifier
     */
    json_item_t *id;
    /**
     * method
     */
    strptr_t method;
    /**
     * json item parameters
     */
    json_item_t *params;
    /**
     * json item result
     */
    json_item_t *result;
    /**
     * error code
     */
    int64_t error_code;
    /**
     * error message
     */
    strptr_t error_message;
    /**
     * json item error data
     */
    json_item_t *error_data;
    /**
     * json rpc object
     */
    json_t *json;
} jsonrpc_t;

/**
 * @brief json error enumerator
 */
typedef struct {
    /**
     * owner
     */
    jsonrpc_t *jsonrpc;
    /**
     * error code
     */
    int errcode;
} jsonrpc_enum_t;

/**
 * callback function for json rpc parser
 */
typedef json_t *(*json_parse_h) (const char*, size_t);

/**
 * internal function for parsing request
 */
int jsonrpc_parse_request_intr (const char *json_str, size_t json_str_len, json_parse_h on_parse, jsonrpc_t *jsonrpc);

/**
 * internal function for parsing response
 */
int jsonrpc_parse_response_intr (const char *json_str, size_t json_str_len, json_parse_h on_parse, jsonrpc_t *jsonrpc);

/**
 * @brief json rpc request parser
 * @param json_str json string
 * @param json_str_len json string length
 * @param jsonrpc json rpc object
 * @return json rpc error code
 */
static inline int jsonrpc_parse_request_len (const char *json_str, size_t json_str_len, jsonrpc_t *jsonrpc) { return jsonrpc_parse_request_intr(json_str, json_str_len, json_parse_len, jsonrpc); };

/**
 * @brief json rpc request parser
 * @param json_str json string
 * @param jsonrpc json rpc object
 * @return json rpc error code
 */
static inline int jsonrpc_parse_request (const char *json_str, jsonrpc_t *jsonrpc) { return jsonrpc_parse_request_intr(json_str, strlen(json_str), json_parse_len, jsonrpc); };

/**
 * @brief json rpc response parser
 * @param json_str json string
 * @param json_str_len json string length
 * @param jsonrpc json rpc object
 * @return json rpc error code
 */
static inline int jsonrpc_parse_response_len (const char *json_str, size_t json_str_len, jsonrpc_t *jsonrpc) { return jsonrpc_parse_response_intr(json_str, json_str_len, json_parse_len, jsonrpc); };

/**
 * @brief json rpc response parser
 * @param json_str json string
 * @param jsonrpc json rpc object
 * @return json rpc error code
 */
static inline int jsonrpc_parse_response (const char *json_str, jsonrpc_t *jsonrpc) { return jsonrpc_parse_response_intr(json_str, strlen(json_str), json_parse_len, jsonrpc); };

/**
 * callback for creating json rpc requests or responses
 */
typedef int (*jsonrpc_h) (strbuf_t*, void *userdata);

/**
 * @brief set global json rpc version
 * @param ver version
 */
void jsonrpc_setver (jsonrpc_ver_t ver);

/**
 * @brief create json rpc request
 * @param buf string buffer
 * @param method
 * @param method_len method length
 * @param id
 * @param id_len
 * @param on_params callback function for creating parameters
 * @param userdata
 * @return value of \b on_params
 * can set id and id_len by macros
 * <ul>
 *  <li> JSONRPC_ID_INT
 *  <li> JSONRPC_ID_STR
 *  <li> JSONRPC_ID_NULL
 * </ul>
 */
int jsonrpc_request (strbuf_t *buf, const char *method, size_t method_len, intptr_t id, int id_len, jsonrpc_h on_params, void *userdata);

/**
 * @brief create json rpc response
 * @param buf string buffer
 * @param id
 * @param id_len
 * @param on_result callback function for creating json rpc result
 * @param userdata
 * @return value of \b on_result
 * can set id and id_len by macros
 * <ul>
 *  <li> JSONRPC_ID_INT
 *  <li> JSONRPC_ID_STR
 *  <li> JSONRPC_ID_NULL
 * </ul>
 */
int jsonrpc_response (strbuf_t *buf, intptr_t id, int id_len, jsonrpc_h on_result, void *userdata);

/**
 * @brief create json rpc error message
 * @param buf string buffer
 * @param code error code
 * @param message error message
 * @param message_len error message length
 * @param id
 * @param id_len
 * @param on_data callback function for creating error data, can be NULL
 * @param userdata
 */
void jsonrpc_error (strbuf_t *buf, int code, const char *message, size_t message_len, intptr_t id, int id_len, jsonrpc_h on_data, void *userdata);

/**
 * @brief create standard error message
 * @param buf string buffer
 * @param code error code
 * <ul>
 *  <li> JSONRPC_PARSE_ERROR
 *  <li> JSONRPC_INVALID_REQUEST
 *  <li> JSONRPC_METHOD_NOT_FOUND
 *  <li> JSONRPC_INVALID_PARAMS
 *  <li> JSONRPC_INTERNAL_ERROR
 * </ul>
 * @param id
 * @param id_len
 * can set id and id_len by macros
 * <ul>
 *  <li> JSONRPC_ID_INT
 *  <li> JSONRPC_ID_STR
 *  <li> JSONRPC_ID_NULL
 * </ul>
 */
void jsonrpc_stderror (strbuf_t *buf, int code, intptr_t id, int id_len);

/**
 * callback function handler
 */
typedef void (*jsonrpc_method_h) (strbuf_t *buf, json_item_t **params, size_t params_len, intptr_t id, int id_len);

/**
 * @brief method array
 */
typedef struct {
    /**
     * method name
     */
    strptr_t method;
    /**
     * method handler
     */
    jsonrpc_method_h handle;
} jsonrpc_method_t;

/**
 * @brief execute callback function from \b methods based on method from json rpc request
 * @param buf json rpc request
 * @param methods method array
 */
void jsonrpc_execute (strbuf_t *buf, jsonrpc_method_t *methods);

#endif // __LIBEX_JSON_H__
