#include "msg.h"

int msg_alloc (msgbuf_t *msg, uint32_t method, const char *cookie, size_t cookie_len, uint32_t chunk_size) {
    uint32_t len = cookie_len + sizeof(uint32_t) * 4;
    uint32_t bufsize = (len / chunk_size) * chunk_size + chunk_size;
    if (len == bufsize) bufsize += chunk_size;
    char *buf = malloc(bufsize);
    if (!buf) return -1;
    msg->ptr = msg->pc = buf;
    msg->ptr[0] = '\0';
    msg->len = len;
    msg->bufsize = bufsize;
    msg->chunk_size = chunk_size;
    *(uint32_t*)msg->ptr = sizeof(uint32_t);
    msg->pc += sizeof(uint32_t);
    return 0 == msg_setui32(msg, method) && 0 == msg_setstr(msg, cookie, cookie_len) ? 0 : -1;
}

int msg_setbuf (msgbuf_t *msg, void *src, uint32_t src_len) {
    char *buf = msg->ptr;
    uint32_t nstr_len = msg->len + src_len;
    errno = 0;
    if (nstr_len >= msg->bufsize) {
        uint32_t nbufsize = (nstr_len / msg->chunk_size) * msg->chunk_size + msg->chunk_size;
        uintptr_t pc_len = (uintptr_t)msg->pc - (uintptr_t)msg->ptr;
        buf = realloc(buf, nbufsize);
        if (!buf) return -1;
        msg->ptr = buf;
        msg->pc = msg->ptr + pc_len;
        msg->bufsize = nbufsize;
    }
    memcpy(msg->pc, src, src_len);
    msg->len = nstr_len;
    msg->pc += src_len;
    *(uint32_t*)msg->ptr = nstr_len;
    return 0;
}

int msg_setstr (msgbuf_t *msg, const char *src, size_t src_len) {
    char *buf = msg->ptr;
    uint32_t nstr_len = msg->len + src_len + sizeof(uint32_t) + sizeof(uint32_t);
    errno = 0;
    if (nstr_len >= msg->bufsize) {
        uint32_t nbufsize = (nstr_len / msg->chunk_size) * msg->chunk_size + msg->chunk_size;
        uintptr_t pc_len = (uintptr_t)msg->pc - (uintptr_t)msg->ptr;
        buf = realloc(buf, nbufsize);
        if (!buf) return -1;
        msg->ptr = buf;
        msg->pc = msg->ptr + pc_len;
        msg->bufsize = nbufsize;
    }
    *(uint32_t*)msg->pc = src_len;
    memcpy(msg->pc + sizeof(uint32_t), src, src_len);
    msg->pc += sizeof(uint32_t) + src_len;
    memset(msg->pc, 0, sizeof(uint32_t));
    msg->pc += sizeof(uint32_t);
    msg->len = nstr_len;
    *(uint32_t*)msg->ptr = nstr_len;
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
