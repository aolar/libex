#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "../include/libex/file.h"
#include "../include/libex/json.h"
#if 0
void test_json1 () {
    int fd = open("./json_1.txt", O_RDONLY);
    if (-1 != fd) {
        struct stat st;
        if (-1 != fstat(fd, &st) && st.st_size > 0) {
            char *buf = malloc(st.st_size+1);
            if (st.st_size == read(fd, buf, st.st_size)) {
                json_t *json = json_parse_len(buf, st.st_size);
                json_item_t *j_status = json_find(json->data.o, CONST_STR_LEN("status"), JSON_STRING);
                if (j_status) {
                    char *s = strndup(j_status->data.s.ptr, j_status->data.s.len);
                    printf("%s\n", s);
                    free(s);
                }
                json_free(json);
            }
            free(buf);
        }
        close(fd);
    }
}

void test_json2 () {
    strbuf_t buf, buf1;
    strbufalloc(&buf, 1024, 1024);
    strbufalloc(&buf1, 128, 128);
    strbuf_escape(&buf1, CONST_STR_LEN("\n Грузите апельсины бочками \n"));
    json_begin_object(&buf);
    json_add_str(&buf, CONST_STR_LEN("name"), buf1.ptr, buf1.len, JSON_NEXT);
    json_add_double(&buf, CONST_STR_LEN("price"), 2.65498727, JSON_END);
    json_end_object(&buf);
    printf("%s\n", buf.ptr);
    free(buf.ptr);
    free(buf1.ptr);
}

static int on_cmp (void *x, void *y) {
    intptr_t a = (intptr_t)x, b = (intptr_t)y;
    if (a > b) return 1;
    if (a < b) return -1;
    return 0;
}

static void *on_cp (void *key) {
    return key;
}

static int on_add_item (tree_item_t *item, strbuf_t *buf, void *userdata) {
    json_add_int(buf, CONST_STR_NULL, (intptr_t)item->key, JSON_END);
    return JSON_INSERTED;
}

void test_json3 () {
    rbtree_t *tree = rbtree_alloc(on_cmp, on_cp, NULL, RBT_UNIQUE);
    for (intptr_t i = 0; i < 16; ++i)
        rbtree_add(tree, (void*)i);
    strbuf_t buf;
    strbufalloc(&buf, 64, 64);
    json_add_tree(&buf, CONST_STR_NULL, tree, on_add_item, NULL, JSON_END);
    buf.ptr[buf.len] = '\0';
    printf("%s\n", buf.ptr);
    free(buf.ptr);
    rbtree_free(tree);
}

static void on_where (strbuf_t *buf, strptr_t *what) {
    if (0 == cmpstr(what->ptr, what->len, CONST_STR_LEN("Rio de Janeiro")))
        strbufadd(buf, CONST_STR_LEN("Brazil"));
    else
        strbufadd(buf, CONST_STR_LEN("i don't know"));
}

static void json_where (strbuf_t *buf, json_item_t **params, size_t params_len, intptr_t id, int id_len) {
    if (params_len != 1 || params[0]->type != JSON_STRING) {
        jsonrpc_stderror(buf, JSONRPC_INVALID_PARAMS, id, id_len);
        return;
    }
    jsonrpc_response(buf, id, id_len, (jsonrpc_h)on_where, &params[0]->data.s);
}

static void on_swap (strbuf_t *buf, json_item_t **p) {
    json_begin_object(buf);
    json_add_int(buf, CONST_STR_LEN("1"), p[0]->data.i, JSON_NEXT);
    json_add_int(buf, CONST_STR_LEN("2"), p[1]->data.i, JSON_END);
    json_end_object(buf);
}

static void json_swap (strbuf_t *buf, json_item_t **params, size_t params_len, intptr_t id, int id_len) {
    if (2 == params_len && params[0]->type == JSON_INTEGER && params[1]->type == JSON_INTEGER) {
        json_item_t *p [2] = { params[1], params[0] };
        jsonrpc_response(buf, id, id_len, (jsonrpc_h)on_swap, p);
        return;
    }
    jsonrpc_stderror(buf, JSONRPC_INVALID_PARAMS, id, id_len);
}

static void on_where_params (strbuf_t *buf, strptr_t *town) {
    json_add_str(buf, CONST_STR_NULL, town->ptr, town->len, JSON_END);
}

static void on_swap_params (strbuf_t *buf, int *vals) {
    json_add_int(buf, CONST_STR_NULL, vals[0], JSON_NEXT);
    json_add_int(buf, CONST_STR_NULL, vals[1], JSON_END);
}

jsonrpc_method_t methods [] = {
    { .method = CONST_STR_INIT("where"), .handle = json_where },
    { .method = CONST_STR_INIT("swap"), .handle = json_swap },
    { .method = CONST_STR_INIT_NULL }
};

void test_json4 () {
    char *str;
    strbuf_t buf = { .ptr = NULL, .len = 0 };
    strptr_t town = CONST_STR_INIT("Rio de Janeiro");
    int vals [] = { 1, 2 };
    strbufalloc(&buf, 64, 64);
    jsonrpc_request(&buf, CONST_STR_LEN("where"), JSONRPC_ID_INT(1), (jsonrpc_h)on_where_params, &town);
    str = strndup(buf.ptr, buf.len);
    printf("request: %s\n", str);
    free(str);
    jsonrpc_execute(&buf, methods);
    str = strndup(buf.ptr, buf.len);
    printf("response: %s\n", str);
    free(str);
    jsonrpc_request(&buf, CONST_STR_LEN("swap"), JSONRPC_ID_INT(2), (jsonrpc_h)on_swap_params, &vals);
    str = strndup(buf.ptr, buf.len);
    printf("request: %s\n", str);
    free(str);
    jsonrpc_execute(&buf, methods);
    str = strndup(buf.ptr, buf.len);
    printf("response: %s\n", str);
    free(str);
    jsonrpc_request(&buf, CONST_STR_LEN("where"), JSONRPC_ID_INT(1), NULL, &town);
    str = strndup(buf.ptr, buf.len);
    printf("request: %s\n", str);
    free(str);
    jsonrpc_execute(&buf, methods);
    str = strndup(buf.ptr, buf.len);
    printf("response: %s\n", str);
    free(str);
    
    free(buf.ptr);
}
#endif

void test_json5 () {
    str_t *str = load_all_file("./json_4.txt", 2048, 8192);
    json_t *json = json_parse_len(str->ptr, str->len);
    if (json) {
        printf("Ok\n");
        json_free(json);
    }
    printf("%d %s\n", errno, strerror(errno));
    free(str);
}

int main (int argc, const char *argv[]) {
//    test_json1();
//    test_json2();
//    test_json3();
//    test_json4();
    test_json5();
    return 0;
}
