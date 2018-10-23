#include "msg.h"

msg_allocator_h msg_alloc = malloc;
msg_deallocator_h msg_free = free;
msg_reallocator_h msg_realloc = realloc;

static int allocate (msgbuf_t *msg, uint32_t len, uint32_t chunk_size) {
    uint32_t bufsize = (len / chunk_size) * chunk_size + chunk_size;
    if (len == bufsize) bufsize += chunk_size;
    char *buf = msg_alloc(bufsize);
    if (!buf) return -1;
    msg->ptr = buf;
    msg->len = sizeof(uint32_t);
    msg->bufsize = bufsize;
    msg->chunk_size = chunk_size;
    *(uint32_t*)msg->ptr = sizeof(uint32_t);
    msg->pc = msg->ptr + sizeof(uint32_t);
    return 0;
}

int msg_create_request (msgbuf_t *msg, uint32_t method, const char *cookie, size_t cookie_len, uint32_t len, uint32_t chunk_size) {
    len = cookie_len + sizeof(uint32_t) * 4 + len;
    if (-1 == allocate(msg, len, chunk_size))
        return -1;
    return 0 == msg_setui32(msg, method) && 0 == msg_setstr(msg, cookie, cookie_len) ? 0 : -1;
}

int msg_create_response (msgbuf_t *msg, int code, uint32_t len, uint32_t chunk_size) {
    len = sizeof(uint32_t) * 2 + len;
    if (-1 == allocate(msg, len, chunk_size))
        return -1;
    return msg_seti32(msg, code);
}

int msg_setbuf (msgbuf_t *msg, void *src, uint32_t src_len) {
    char *buf = msg->ptr;
    uint32_t nstr_len = msg->len + src_len;
    errno = 0;
    if (nstr_len >= msg->bufsize) {
        uint32_t nbufsize = (nstr_len / msg->chunk_size) * msg->chunk_size + msg->chunk_size;
        uintptr_t pc_len = (uintptr_t)msg->pc - (uintptr_t)msg->ptr;
        buf = msg_realloc(buf, nbufsize);
        if (!buf) return -1;
        msg->ptr = buf;
        msg->pc = msg->ptr + pc_len;
        msg->bufsize = nbufsize;
    }
    memcpy(msg->pc, src, src_len);
    msg->pc += src_len;
    msg->len = nstr_len;
    *(uint32_t*)msg->ptr = msg->len;
    return 0;
}

int msg_setstr (msgbuf_t *msg, const char *src, size_t src_len) {
    char *buf = msg->ptr;
    uint32_t nstr_len = msg->len + src_len + sizeof(uint32_t) * 2;
    errno = 0;
    if (nstr_len >= msg->bufsize) {
        uint32_t nbufsize = (nstr_len / msg->chunk_size) * msg->chunk_size + msg->chunk_size;
        uintptr_t pc_len = (uintptr_t)msg->pc - (uintptr_t)msg->ptr;
        buf = msg_realloc(buf, nbufsize);
        if (!buf) return -1;
        msg->ptr = buf;
        msg->pc = msg->ptr + pc_len;
        msg->bufsize = nbufsize;
    }
    *(uint32_t*)msg->pc = src_len;
    memcpy(msg->pc + sizeof(uint32_t), src, src_len);
    msg->pc += src_len + sizeof(uint32_t);
    *(uint32_t*)msg->pc = 0;
    msg->pc += sizeof(uint32_t);
    msg->len = nstr_len;
    *(uint32_t*)msg->ptr = msg->len;
    return 0;
}

typedef struct {
    uintptr_t pc;
    msgbuf_t *msg;
    msg_item_h on_item;
    void *userdata;
    int rc;
} msg_list_item_t;

static int on_list_item (list_item_t *li, msg_list_item_t *m) {
    int rc = m->on_item(m->msg, li->ptr, m->userdata);
    if (0 != errno) {
        m->rc = -1;
        return ENUM_BREAK;
    }
    if (MSG_INSERTED == rc) {
        *(uint32_t*)(m->msg->ptr + m->pc) += 1;
        return ENUM_CONTINUE;
    }
    return rc;
}

int msg_setlist (msgbuf_t *msg, list_t *lst, msg_item_h fn, void *userdata) {
    msg_list_item_t m = { .pc = (uintptr_t)msg->pc - (uintptr_t)msg->ptr, .msg = msg, .on_item = fn, .userdata = userdata, .rc = 0 };
    if (-1 == msg_setui32(msg, 0))
        return -1;
    lst_enum(lst, (list_item_h)on_list_item, &m, ENUM_STOP_IF_BREAK);
    return m.rc;
}

int msg_load_request (msgbuf_t *msg, char *buf, size_t buflen) {
    uint32_t len;
    errno = 0;
    msg_clear(msg);
    msg->ptr = msg->pc = buf;
    msg->len = msg->bufsize = buflen;
    msg->chunk_size = 0;
    if (-1 == msg_getui32(msg, &len)) {
        msg->ptr = NULL;
        return -1;
    }
    if (len != buflen) {
        errno = EFAULT;
        msg->ptr = NULL;
        return -1;
    }
    if (-1 == msg_getui32(msg, &msg->method)) {
        msg->ptr = NULL;
        return -1;
    }
    if (-1 == msg_getstr(msg, &msg->cookie)) {
        msg->ptr = NULL;
        return -1;
    }
    return 0;
}

int msg_load_response (msgbuf_t *msg, char *buf, size_t buflen) {
    uint32_t len;
    errno = 0;
    msg_clear(msg);
    msg->ptr = msg->pc = buf;
    msg->len = msg->bufsize = buflen;
    msg->chunk_size = 0;
    if (-1 == msg_getui32(msg, &len))
        return -1;
    if (len != buflen) {
        errno = EFAULT;
        return -1;
    }
    if (-1 == msg_getui32(msg, &msg->code))
        return -1;
    if (MSG_OK == msg->code)
        return 0;
    return msg_getstr(msg, &msg->errmsg);
}

int msg_geti8 (msgbuf_t *msg, int8_t *val) {
    errno = 0;
    if (msg->ptr + msg->len < msg->pc + sizeof(int8_t)) {
        errno = EFAULT;
        return -1;
    }
    *val = *(int8_t*)msg->pc;
    msg->pc += sizeof(uint8_t);
    return 0;
}

int msg_geti (msgbuf_t *msg, int *val) {
    errno = 0;
    if (msg->ptr + msg->len < msg->pc + sizeof(int)) {
        errno = EFAULT;
        return -1;
    }
    *val = *(int*)msg->pc;
    msg->pc += sizeof(int);
    return 0;
}

int msg_geti32 (msgbuf_t *msg, int32_t *val) {
    errno = 0;
    if (msg->ptr + msg->len < msg->pc + sizeof(int32_t)) {
        errno = EFAULT;
        return -1;
    }
    *val = *(int32_t*)msg->pc;
    msg->pc += sizeof(int32_t);
    return 0;
}

int msg_getui32 (msgbuf_t *msg, uint32_t *val) {
    errno = 0;
    if (msg->ptr + msg->len < msg->pc + sizeof(uint32_t)) {
        errno = EFAULT;
        return -1;
    }
    *val = *(uint32_t*)msg->pc;
    msg->pc += sizeof(uint32_t);
    return 0;
}

int msg_geti64 (msgbuf_t *msg, int64_t *val) {
    errno = 0;
    if (msg->ptr + msg->len < msg->pc + sizeof(int64_t)) {
        errno = EFAULT;
        return -1;
    }
    *val = *(int64_t*)msg->pc;
    msg->pc += sizeof(int64_t);
    return 0;
}

int msg_getd (msgbuf_t *msg, double *val) {
    errno = 0;
    if (msg->ptr + msg->len < msg->pc + sizeof(double)) {
        errno = EFAULT;
        return -1;
    }
    *val = *(double*)msg->pc;
    msg->pc += sizeof(double);
    return 0;
}

int msg_getstr (msgbuf_t *msg, strptr_t *str) {
    uint32_t len;
    errno = 0;
    if (-1 == msg_getui32(msg, &len)) {
        errno = EFAULT;
        return -1;
    }
    if (msg->ptr + msg->len < msg->pc + len) {
        errno = EFAULT;
        return -1;
    }
    str->len = len;
    str->ptr = msg->pc;
    msg->pc += len + sizeof(uint32_t);
    return 0;
}

int msg_enum (msgbuf_t *msg, msg_item_h fn, void *userdata) {
    uint32_t count;
    if (-1 == msg_getui32(msg, &count))
        return -1;
    for (uint32_t i = 0; i < count; ++i) {
        if (ENUM_CONTINUE != fn(msg, NULL, userdata))
            break;
    }
    return 0 == errno ? 0 : -1;
}

int msg_error (msgbuf_t *msg, int code, const char *str, size_t len) {
    if (!len) len = strlen(str);
    if (-1 == msg_create_response(msg, code, sizeof(uint32_t) * 3 + len, 8))
        return -1;
    return msg_setstr(msg, str, len);
}
