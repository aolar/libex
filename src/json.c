#include "json.h"

enum { JSON_OK, JSON_FIN, JSON_ERROR };
__thread char *json_error_msg = NULL;
const char *delims = "{}[],:";

/*************************************************************************************
  json parser
*************************************************************************************/

static int get_token (json_t *j, strptr_t *token) {
    char *p = j->text_ptr, *q, *e = j->text + j->text_len;
    token->ptr = NULL;
    token->len = 0;
    while (p < e && isspace(*p)) ++p;
    if (p >= e)
        return JSON_FIN;
    q = p;
    if ('"' == *p) {
        token->ptr = p++;
        while (p < e) {
            if (*p == '"')
                break;
            if (*p == '\\')
                ++p;
            ++p;
        }
        if (p >= e)
            return JSON_ERROR;
        j->text_ptr = ++p;
        token->ptr = q;
        token->len = (uintptr_t)p - (uintptr_t)q;
        return JSON_OK;
    }
    switch (*p++) {
        case '{':
        case '}':
        case ',':
        case ':':
        case '[':
        case ']':
            token->ptr = q;
            token->len = 1;
            j->text_ptr = p;
            return JSON_OK;
    }
    while (p < e && ! isspace(*p) && !strchr(delims, *p)) ++p;
    e = p;
    if (e > q) {
        token->ptr = q;
        token->len = (uintptr_t)p - (uintptr_t)q;
        j->text_ptr = p;
    }
    return JSON_OK;
}

// free json objects
static void json_free_object (json_object_t *jo);
static void json_free_array (json_array_t *ja);
static void json_free_item (json_item_t *ji);

static void json_free_object (json_object_t *jo) {
    rbtree_free(jo);
}

static void json_free_array (json_array_t *ja) {
    lst_free(ja);
}

static void json_clear_item (json_item_t *ji) {
    switch (ji->type) {
        case JSON_OBJECT:
            json_free_object(ji->data.o);
            break;
        case JSON_ARRAY:
            json_free_array(ji->data.a);
            break;
        default: break;
    }
}

static void json_free_item (json_item_t *ji) {
    json_clear_item(ji);
    free(ji);
}

void json_free (json_t *j) {
    switch (j->type) {
        case JSON_OBJECT:
            json_free_object(j->data.o);
            break;
        case JSON_ARRAY:
            json_free_array(j->data.a);
            break;
        default: break;
    }
    free(j);
}

int json_str2long (strptr_t *str, int64_t *l) {
    char *s = strndup(str->ptr, str->len), *tail;
    *l = strtoll(s, &tail, 0);
    int ret = *tail != '\0' || errno == ERANGE ? -1 : 0;
    free(s);
    return ret;
}

int json_str2double (strptr_t *str, long double *d) {
    char *s = strndup(str->ptr, str->len), *tail;
    *d = strtold(s, &tail);
    int ret = *tail != '\0' || errno == ERANGE ? -1 : 0;
    free(s);
    return ret;
}

static void json_set_item_value (json_t *j, json_item_t *ji, strptr_t *token) {
    if ('"' == token->ptr[0] && '"' == token->ptr[token->len-1]) {
        token->ptr++;
        token->len -= 2;
        ji->type = JSON_STRING;
        ji->data.s = *token;
    } else
    if (0 == json_str2long(token, &ji->data.i))
        ji->type = JSON_INTEGER;
    else
    if (0 == json_str2double(token, &ji->data.d))
        ji->type = JSON_DOUBLE;
    else
    if (0 == cmpcasestr(token->ptr, token->len, CONST_STR_LEN("TRUE")))
        ji->type = JSON_TRUE;
    else
    if (0 == cmpcasestr(token->ptr, token->len, CONST_STR_LEN("FALSE")))
        ji->type = JSON_FALSE;
    else
    if (0 == cmpcasestr(token->ptr, token->len, CONST_STR_LEN("NULL")))
        ji->type = JSON_NULL;
    else {
        ji->type = JSON_STRING;
        ji->data.s = *token;
    }
}

static json_array_t *json_parse_array (json_t *j, strptr_t *token);
static json_item_t *json_parse_item (json_t *j, strptr_t *token);
static json_object_t *json_parse_object(json_t *j, strptr_t *token);

static json_array_t *json_parse_array (json_t *j, strptr_t *token) {
    json_array_t *a = lst_alloc((free_h)json_free_item);
    while (JSON_OK == get_token(j, token)) {
        json_item_t *ji;
        if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN("]")))
            break;
        ji = calloc(1, sizeof(json_item_t));
        if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN("{"))) {
            ji->type = JSON_OBJECT;
            if (!(ji->data.o = json_parse_object(j, token))) {
                json_free_item(ji);
                goto err;
            }
        } else
            json_set_item_value(j, ji, token);
        lst_adde(a, (void*)ji);
        if (JSON_OK != get_token(j, token)) goto err;
        if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN(",")))
            continue;
        if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN("]")))
            break;
    }
    return a;
err:
    json_free_array(a);
    return NULL;
}

static json_item_t *json_parse_item (json_t *j, strptr_t *token) {
    if ('"' != token->ptr[0] || '"' != token->ptr[token->len-1])
        return NULL;
    token->ptr++;
    token->len -= 2;
    json_item_t *ji = calloc(1, sizeof(json_item_t));
    ji->key = *token;
    if (JSON_OK != get_token(j, token) || 0 != cmpstr(token->ptr, token->len, CONST_STR_LEN(":")) || JSON_OK != get_token(j, token)) {
        json_free_item(ji);
        return NULL;
    }
    if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN("{"))) {
        ji->type = JSON_OBJECT;
        if (!(ji->data.o = json_parse_object(j, token))) {
            json_free_item(ji);
            return NULL;
        }
    } else
    if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN("["))) {
        ji->type = JSON_ARRAY;
        if (!(ji->data.a = json_parse_array(j, token))) {
            json_free_item(ji);
            return NULL;
        }
    } else
    json_set_item_value(j, ji, token);
    return ji;
}

static int on_json_item_compare (void *x, void *y) {
    strptr_t *s1 = (strptr_t*)x, *s2 = (strptr_t*)y;
    return cmpstr(s1->ptr, s1->len, s2->ptr, s2->len);
}

static void *on_json_copy_key (void *key) {
    return key;
}

static void on_json_item_free (void *key, void *value) {
    json_free_item((json_item_t*)value);
}

static json_object_t *json_parse_object(json_t *j, strptr_t *token) {
    json_object_t *o = rbtree_alloc(on_json_item_compare, on_json_copy_key, on_json_item_free, RBT_UNIQUE);
    while (JSON_OK == get_token(j, token)) {
        json_item_t *ji;
        if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN("}")))
            break;
        if (!(ji = json_parse_item(j, token)))
            goto err;
        tree_item_t *n = rbtree_add(o, &ji->key);
        if (EEXIST == errno) {
            json_free_item(ji);
            goto err;
        }
        n->value = ji;
        if (JSON_OK != get_token(j, token))
            goto err;
        if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN(",")))
            continue;
        if (0 == cmpstr(token->ptr, token->len, CONST_STR_LEN("}")))
            break;
    }
    return o;
err:
    json_free_object(o);
    return NULL;
}

json_t *json_parse_len (const char *json_str, size_t json_str_len) {
    json_t *j = calloc(1, sizeof(json_t));
    strptr_t token;
    j->text = j->text_ptr = (char*)json_str;
    j->text_len = json_str_len;
    if (JSON_ERROR == get_token(j, &token)) goto err;
    if (0 == cmpstr(token.ptr, token.len, CONST_STR_LEN("{"))) {
        j->type = JSON_OBJECT;
        if (!(j->data.o = json_parse_object(j, &token))) goto err;
    } else
    if (0 == cmpstr(token.ptr, token.len, CONST_STR_LEN("["))) {
        j->type = JSON_ARRAY;
        if (!(j->data.a = json_parse_array(j, &token))) goto err;
    } else
        goto err;
    return j;
err:
    free(j);
    return NULL;
}

json_t *json_parse (const char *json_str) {
    return json_parse_len(json_str, strlen(json_str));
}

json_item_t *json_find (json_object_t *jo, const char *key, size_t key_len, int type) {
    strptr_t k = { .ptr = (char*)key, .len = key_len };
    tree_item_t *n = rbtree_get(jo, &k);
    if (!n)
        return NULL;
    json_item_t *j = (json_item_t*)n->value;
    if (-1 == type)
        return j;
    return j && type == j->type ? j : NULL;
}

/*************************************************************************************
  create json
*************************************************************************************/

void json_enum_array (json_array_t *lst, json_item_h fn, void *userdata, int flags) {
    list_item_t *li = lst->head;
    if (li) {
        do {
            json_item_t *j = (json_item_t*)li->ptr;
            int n = fn(j, userdata);
            if (ENUM_STOP_IF_BREAK == flags && ENUM_BREAK == n)
                return;
            li = li->next;
        } while (li != lst->head);
    }
}

typedef struct {
    json_item_h fn;
    void *userdata;
} json_enum_object_t;

static int on_object (tree_item_t *item, void *userdata) {
    json_enum_object_t *je = (json_enum_object_t*)userdata;
    return je->fn((json_item_t*)item->value, je->userdata);
}

void json_enum_object (json_object_t *obj, json_item_h fn, void *userdata, int flags) {
    json_enum_object_t je = { .fn = fn, .userdata = userdata };
    rbtree_enum(obj, NULL, on_object, &je, flags);
}

void json_add_key (strbuf_t *buf, const char *key, size_t key_len) {
    strbufadd(buf, CONST_STR_LEN("\""));
    strbufadd(buf, key, key_len);
    strbufadd(buf, CONST_STR_LEN("\":"));
}

void json_add_str (strbuf_t *buf, const char *key, size_t key_len, const char *val, size_t val_len, int is_end) {
    if (key && key_len)
        json_add_key(buf, key, key_len);
    strbufadd(buf, CONST_STR_LEN("\""));
    strbufadd(buf, val, val_len);
    strbufadd(buf, CONST_STR_LEN("\""));
    if (!is_end)
        strbufadd(buf, CONST_STR_LEN(","));
}

void json_add_escape_str (strbuf_t *buf, const char *key, size_t key_len, const char *val, size_t val_len, int is_end) {
    if (key && key_len)
        json_add_key(buf, key, key_len);
    strbufadd(buf, CONST_STR_LEN("\""));
    strbuf_escape(buf, val, val_len);
    strbufadd(buf, CONST_STR_LEN("\""));
    if (!is_end)
        strbufadd(buf, CONST_STR_LEN(","));
}

void json_add_int (strbuf_t *buf, const char *key, size_t key_len, int64_t val, int is_end) {
    char str [48];
    snprintf(str, sizeof(str), LONG_FMT, val);
    if (key && key_len)
        json_add_key(buf, key, key_len);
    strbufadd(buf, str, strlen(str));
    if (!is_end)
        strbufadd(buf, CONST_STR_LEN(","));
}

void json_add_double_p (strbuf_t *buf, const char *key, size_t key_len, long double val, int prec, int is_end) {
    char str [48];
    if (prec <= 0)
        prec = JSON_DOUBLE_PRECISION;
    snprintf(str, sizeof(str), "%.*Lf", prec, val);
    if (key && key_len)
        json_add_key(buf, key, key_len);
    strbufadd(buf, str, strlen(str));
    if (!is_end)
        strbufadd(buf, CONST_STR_LEN(","));
}

void json_add_null (strbuf_t *buf, const char *key, size_t key_len, int is_end) {
    if (key && key_len)
        json_add_key(buf, key, key_len);
    strbufadd(buf, CONST_STR_LEN("null"));
    if (!is_end)
        strbufadd(buf, CONST_STR_LEN(","));
}

void json_add_bool (strbuf_t *buf, const char *key, size_t key_len, int val, int is_end) {
    char *str = 0 == val ? "false" : "true";
    if (key && key_len)
        json_add_key(buf, key, key_len);
    strbufadd(buf, str, strlen(str));
    if (!is_end)
        strbufadd(buf, CONST_STR_LEN(","));
}

void json_open_array (strbuf_t *buf, const char *key, size_t key_len) {
    if (key && key_len)
        json_add_key(buf, key, key_len);
    strbufadd(buf, CONST_STR_LEN("["));
}

void json_close_array (strbuf_t *buf, int is_end) {
    if (is_end)
        strbufadd(buf, CONST_STR_LEN("]"));
    else
        strbufadd(buf, CONST_STR_LEN("],"));
}

void json_add_list (strbuf_t *buf, const char *key, size_t key_len, list_t *list, json_list_item_h on_item, void *userdata, int is_end) {
    if (key && key_len)
        json_open_array(buf, key, key_len);
    if (list) {
        list_item_t *li = list->head;
        if (li) {
            do {
                int inserted = on_item(li, buf, userdata);
                li = li->next;
                if (JSON_INSERTED == inserted && li != list->head)
                    strbufadd(buf, CONST_STR_LEN(","));
            } while (li != list->head);
        }
    }
    if (key && key_len)
        json_close_array(buf, is_end);
    else
    if (JSON_NEXT == is_end)
        strbufadd(buf, CONST_STR_LEN(","));
}

static int on_tree_item (tree_item_t *item, list_t *lst) {
    lst_adde(lst, item);
    return ENUM_CONTINUE;
}

typedef struct {
    json_tree_item_h on_item;
    strbuf_t *buf;
    void *userdata;
} json_tree_item_result_t;

static int on_list_item (list_item_t *item, json_tree_item_result_t *data) {
    int rc = data->on_item((tree_item_t*)item->ptr, data->buf, data->userdata);
    if (JSON_INSERTED == rc && item->next != item->list->head)
        strbufadd(data->buf, CONST_STR_LEN(","));
    return ENUM_CONTINUE;
}

void json_add_tree (strbuf_t *buf, const char *key, size_t key_len, rbtree_t *tree, json_tree_item_h on_item, void *userdata, int is_end) {
    json_tree_item_result_t data = { .on_item = on_item, .buf = buf, .userdata = userdata };
    list_t *lst = lst_alloc(NULL);
    rbtree_enum(tree, NULL, (tree_item_h)on_tree_item, (void*)lst, 0);
    if (key && key_len)
        json_open_array(buf, key, key_len);
    else
        json_begin_array(buf);
    lst_enum(lst, (list_item_h)on_list_item, &data, 0);
    lst_free(lst);
    json_end_array(buf);
    if (JSON_NEXT == is_end)
        strbufadd(buf, CONST_STR_LEN(","));
}

void json_open_object (strbuf_t *buf, const char *key, size_t key_len) {
    if (key && key_len)
        json_add_key(buf, key, key_len);
    strbufadd(buf, CONST_STR_LEN("{"));
}

void json_close_object (strbuf_t *buf, int is_end) {
    if (is_end)
        strbufadd(buf, CONST_STR_LEN("}"));
    else
        strbufadd(buf, CONST_STR_LEN("},"));
}

/*************************************************************************************
  json rpc
*************************************************************************************/
const char *jsonrpc_version = "1.0";

static int jsonrpc_parse_id (json_item_t *ji, jsonrpc_enum_t *data) {
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("id"))) {
        if (JSON_INTEGER == ji->type || JSON_STRING == ji->type)
            data->jsonrpc->id = ji;
        else {
            data->errcode = JSONRPC_INVALID_REQUEST;
            return -1;
        }
    }
    return 0;
}

static int jsonrpc_parse_ver (json_item_t *ji, jsonrpc_enum_t *data) {
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("jsonrpc"))) {
        if (JSON_STRING == ji->type) {
            if (0 == cmpstr(ji->data.s.ptr, ji->data.s.len, CONST_STR_LEN("1.0")))
                data->jsonrpc->ver = JSONRPC_V10;
            else
            if (0 == cmpstr(ji->data.s.ptr, ji->data.s.len, CONST_STR_LEN("2.0")))
                data->jsonrpc->ver = JSONRPC_V20;
            else {
                data->errcode = JSONRPC_INVALID_REQUEST;
                return -1;
            }
        } else {
            data->errcode = JSONRPC_INVALID_REQUEST;
            return -1;
        }
    }
    return 0;
}

static int jsonrpc_parse_method (json_item_t *ji, jsonrpc_enum_t *data) {
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("method"))) {
        if (JSON_STRING == ji->type)
            data->jsonrpc->method = ji->data.s;
        else {
            data->errcode = JSONRPC_METHOD_NOT_FOUND;
            return -1;
        }
    }
    return 0;
}

static int jsonrpc_parse_error_code (json_item_t *ji, jsonrpc_enum_t *data) {
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("code"))) {
        if (JSON_INTEGER == ji->type)
            data->jsonrpc->error_code = ji->data.i;
        else {
            data->errcode = JSONRPC_INVALID_REQUEST;
            return -1;
        }
    }
    return 0;
}

static int jsonrpc_parse_error_message (json_item_t *ji, jsonrpc_enum_t *data) {
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("message"))) {
        if (JSON_STRING == ji->type)
            data->jsonrpc->error_message = ji->data.s;
        else {
            data->errcode = JSONRPC_INVALID_REQUEST;
            return -1;
        }
    }
    return 0;
}

static int on_parse_error (tree_item_t *item, jsonrpc_enum_t *data) {
    json_item_t *ji = (json_item_t*)item->value;
    if (-1 == jsonrpc_parse_error_code(ji, data))
        return -1;
    if (-1 == jsonrpc_parse_error_message(ji, data))
        return -1;
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("data")))
        data->jsonrpc->error_data = ji;
    return ENUM_CONTINUE;
}

static int jsonrpc_parse_error (json_item_t *ji, jsonrpc_enum_t *data) {
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("error"))) {
        if (JSON_OBJECT != ji->type) {
            data->errcode = JSONRPC_INVALID_REQUEST;
            return -1;
        }
        rbtree_enum(ji->data.o, NULL, (tree_item_h)on_parse_error, (void*)data, ENUM_STOP_IF_BREAK);
        if (JSONRPC_OK != data->errcode)
            return data->errcode;
        if (!data->jsonrpc->error_code || !data->jsonrpc->error_message.ptr) {
            data->errcode = JSONRPC_INVALID_REQUEST;
            return -1;
        }
    }
    return 0;
}

static int on_jsonrpc_parse_request (tree_item_t *item, jsonrpc_enum_t *data) {
    json_item_t *ji = (json_item_t*)item->value;
    if (-1 == jsonrpc_parse_id(ji, data)) {
        data->errcode = JSONRPC_INVALID_REQUEST;
        return ENUM_BREAK;
    }
    if (-1 == jsonrpc_parse_ver(ji, data)) {
        data->errcode = JSONRPC_INVALID_REQUEST;
        return ENUM_BREAK;
    }
    if (-1 == jsonrpc_parse_method(ji, data)) {
        data->errcode = JSONRPC_INVALID_REQUEST;
        return ENUM_BREAK;
    }
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("params")))
        data->jsonrpc->params = ji;
    return ENUM_CONTINUE;
}

static int on_jsonrpc_parse_response (tree_item_t *item, jsonrpc_enum_t *data) {
    json_item_t *ji = (json_item_t*)item->value;
    if (-1 == jsonrpc_parse_id(ji, data))
        return ENUM_BREAK;
    if (-1 == jsonrpc_parse_ver(ji, data))
        return ENUM_BREAK;
    if (-1 == jsonrpc_parse_error(ji, data))
        return ENUM_BREAK;
    if (0 == cmpstr(ji->key.ptr, ji->key.len, CONST_STR_LEN("result")))
        data->jsonrpc->result = ji;
    return ENUM_CONTINUE;
}

int jsonrpc_parse_request_intr (const char *json_str, size_t json_str_len, json_parse_h on_parse, jsonrpc_t *jsonrpc) {
    int rc = JSONRPC_OK;
    jsonrpc_enum_t data = { .jsonrpc = jsonrpc, .errcode = JSONRPC_OK };
    if (!(jsonrpc->json = on_parse(json_str, json_str_len)))
        return JSONRPC_PARSE_ERROR;
    if (JSON_OBJECT != jsonrpc->json->type) {
        rc = JSONRPC_INVALID_REQUEST;
        goto err;
    }
    rbtree_enum(jsonrpc->json->data.o, NULL, (tree_item_h)on_jsonrpc_parse_request, (void*)&data, ENUM_STOP_IF_BREAK);
    if (data.errcode != JSONRPC_OK)
        return data.errcode;
    if (!jsonrpc->id || (jsonrpc->id->type != JSON_INTEGER && jsonrpc->id->type != JSON_STRING) || jsonrpc->ver == JSONRPC_VNONE || !jsonrpc->method.len)
        return JSONRPC_INVALID_REQUEST;
    if (!jsonrpc->params || jsonrpc->params->type != JSON_ARRAY)
        return JSONRPC_INVALID_PARAMS;
    return JSONRPC_OK;
err:
    if (jsonrpc->json) {
        json_free(jsonrpc->json);
        jsonrpc->json = NULL;
    }
    return rc;
}

int jsonrpc_parse_response_intr (const char *json_str, size_t json_str_len, json_parse_h on_parse, jsonrpc_t *jsonrpc) {
    int rc;
    jsonrpc_enum_t data = { .jsonrpc = jsonrpc, .errcode = JSONRPC_OK };
    if (!(jsonrpc->json = on_parse(json_str, json_str_len)))
        return JSONRPC_PARSE_ERROR;
    if (JSON_OBJECT != jsonrpc->json->type) {
        rc = JSONRPC_INVALID_REQUEST;
        goto err;
    }
    rbtree_enum(jsonrpc->json->data.o, NULL, (tree_item_h)on_jsonrpc_parse_response, (void*)&data, ENUM_STOP_IF_BREAK);
    if (data.errcode != JSONRPC_OK)
        return data.errcode;
    if (!jsonrpc->id || jsonrpc->ver == JSONRPC_VNONE || (jsonrpc->result && jsonrpc->error_message.ptr) || (!jsonrpc->result && !jsonrpc->error_message.ptr))
        return JSONRPC_INVALID_REQUEST;
    return JSONRPC_OK;
err:
    if (jsonrpc->json) {
        json_free(jsonrpc->json);
        jsonrpc->json = NULL;
    }
    return rc;
}
void jsonrpc_setver (jsonrpc_ver_t ver) {
    switch (ver) {
        case JSONRPC_V10: jsonrpc_version = "1.0"; break;
        case JSONRPC_V20: jsonrpc_version = "2.0"; break;
        default: break;
    }
}

static void jsonrpc_id (strbuf_t *buf, intptr_t id, int id_len, int is_end) {
    if (id_len > 0)
        json_add_str(buf, CONST_STR_LEN("id"), (const char*)id, id_len, is_end);
    else
    if (id_len == -1)
        json_add_int(buf, CONST_STR_LEN("id"), id, is_end);
    else
    if (id == 0)
        json_add_null(buf, CONST_STR_LEN("id"), is_end);
}

int jsonrpc_request (strbuf_t *buf, const char *method, size_t method_len, intptr_t id, int id_len, jsonrpc_h on_params, void *userdata) {
    int rc = 0;
    buf->len = 0;
    json_begin_object(buf);
    json_add_str(buf, CONST_STR_LEN("jsonrpc"), jsonrpc_version, 3, JSON_NEXT);
    json_add_str(buf, CONST_STR_LEN("method"), method, method_len, JSON_NEXT);
    json_open_array(buf, CONST_STR_LEN("params"));
    if (on_params)
        rc = on_params(buf, userdata);
    json_close_array(buf, JSON_NEXT);
    jsonrpc_id(buf, id, id_len, JSON_END);
    json_end_object(buf);
    return rc;
}

int jsonrpc_response (strbuf_t *buf, intptr_t id, int id_len, jsonrpc_h on_result, void *userdata) {
    buf->len = 0;
    json_begin_object(buf);
    json_add_key(buf, CONST_STR_LEN("result"));
    int rc = on_result(buf, userdata);
    strbufadd(buf, CONST_STR_LEN(","));
    json_add_null(buf, CONST_STR_LEN("error"), JSON_NEXT);
    jsonrpc_id(buf, id, id_len, JSON_END);
    json_end_object(buf);
    return rc;
}

void jsonrpc_error (strbuf_t *buf, int code, const char *message, size_t message_len, intptr_t id, int id_len, jsonrpc_h on_data, void *userdata) {
    buf->len = 0;
    json_begin_object(buf);
    json_add_null(buf, CONST_STR_LEN("result"), JSON_NEXT);
    json_open_object(buf, CONST_STR_LEN("error"));
    json_add_int(buf, CONST_STR_LEN("code"), code, JSON_NEXT);
    json_add_str(buf, CONST_STR_LEN("message"), message, 0 == message_len ? strlen(message) : message_len, JSON_END);
    json_close_object(buf, JSON_NEXT);
    if (on_data)
        on_data(buf, userdata);
    jsonrpc_id(buf, id, id_len, JSON_END);
    json_end_object(buf);
}

#define JSONRPC_PARSE_ERROR_STR "Invalid JSON was received by the server."
#define JSONRPC_INVALID_REQUEST_STR "The JSON sent is not a valid Request object."
#define JSONRPC_METHOD_NOT_FOUND_STR "The method does not exist."
#define JSONRPC_INVALID_PARAMS_STR "Invalid method parameters."
#define JSONRPC_INTERNAL_ERROR_STR "Internal JSON-RPC error."

void jsonrpc_stderror (strbuf_t *buf, int code, intptr_t id, int id_len) {
    switch (code) {
        case JSONRPC_PARSE_ERROR:
            jsonrpc_error(buf, JSONRPC_PARSE_ERROR, CONST_STR_LEN(JSONRPC_PARSE_ERROR_STR), id, id_len, NULL, NULL);
            break;
        case JSONRPC_INVALID_REQUEST:
            jsonrpc_error(buf, JSONRPC_INVALID_REQUEST, CONST_STR_LEN(JSONRPC_INVALID_REQUEST_STR), id, id_len, NULL, NULL);
            break;
        case JSONRPC_METHOD_NOT_FOUND:
            jsonrpc_error(buf, JSONRPC_METHOD_NOT_FOUND, CONST_STR_LEN(JSONRPC_METHOD_NOT_FOUND_STR), id, id_len, NULL, NULL);
            break;
        case JSONRPC_INVALID_PARAMS:
            jsonrpc_error(buf, JSONRPC_INVALID_PARAMS, CONST_STR_LEN(JSONRPC_INVALID_PARAMS_STR), id, id_len, NULL, NULL);
            break;
        case JSONRPC_INTERNAL_ERROR:
            jsonrpc_error(buf, JSONRPC_INTERNAL_ERROR, CONST_STR_LEN(JSONRPC_INTERNAL_ERROR_STR), id, id_len, NULL, NULL);
            break;
        default:
            break;
    }
}

static int get_id (json_item_t *item, intptr_t *id, int *id_len) {
    switch (item->type) {
        case JSON_INTEGER:
            *id = item->data.i;
            *id_len = -1;
            return 0;
        case JSON_STRING:
            *id = (intptr_t)strndup(item->data.s.ptr, item->data.s.len);
            *id_len = item->data.s.len;
            return 0;
        case JSON_NULL:
            *id = 0;
            *id_len = 0;
            return 0;
        default:
            return -1;
    }
}

typedef struct {
    json_item_t **params;
    size_t params_len;
} list_to_array_t;

static int on_list_to_array (json_item_t *li, list_to_array_t *la) {
    la->params[la->params_len++] = li;
    return ENUM_CONTINUE;
}

void jsonrpc_execute (strbuf_t *buf, jsonrpc_method_t *methods) {
    int rc;
    jsonrpc_t jsonrpc;
    memset(&jsonrpc, 0, sizeof(jsonrpc_t));
    if (JSONRPC_OK == (rc = jsonrpc_parse_request_len(buf->ptr, buf->len, &jsonrpc))) {
        intptr_t id = 0;
        int id_len = 0;
        jsonrpc_method_t *m = methods;
        if (0 == get_id(jsonrpc.id, &id, &id_len)) {
            while (m->method.ptr && m->method.len && 0 != cmpstr(m->method.ptr, m->method.len, jsonrpc.method.ptr, jsonrpc.method.len))
                m++;
            if (m->method.ptr && m->method.len) {
                list_to_array_t la = { .params = calloc(jsonrpc.params->data.a->len, sizeof(json_item_t*)), .params_len = 0 };
                json_enum_array(jsonrpc.params->data.a, (json_item_h)on_list_to_array, &la, 0);
                m->handle(buf, la.params, la.params_len, id, id_len);
                free(la.params);
            } else
                jsonrpc_stderror(buf, JSONRPC_METHOD_NOT_FOUND, id, id_len);
            if (id_len > 0)
                free((void*)id);
        } else
            jsonrpc_stderror(buf, JSONRPC_PARSE_ERROR, 0, 0);
    } else
        jsonrpc_stderror(buf, rc, 0, 0);
    if (jsonrpc.json)
        json_free(jsonrpc.json);
}
