#include "unet.h"

int unet_bind (const char *sock_file) {
    struct sockaddr_un addr;
    int fd_listener;
    if ((fd_listener = socket(AF_UNIX, SOCK_STREAM, 0)) < 0)
        return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_file, sizeof(addr.sun_path)-1);
    unlink(sock_file);
    if (bind(fd_listener, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd_listener);
        return -1;
    }
    if (listen(fd_listener, 64) < 0) {
        close(fd_listener);
        return -1;
    }
    return fd_listener;
}

int unet_connect (const char *sock_file) {
    int fd;
    struct sockaddr_un addr;
    if (-1 == (fd = socket(AF_UNIX, SOCK_STREAM, 0)))
        return -1;
    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sock_file, sizeof(addr.sun_path)-1);
    if (-1 == connect(fd, (struct sockaddr*)&addr, sizeof(addr))) {
        close(fd);
        return -1;
    }
    return fd;
}

int unet_read (int fd, msgbuf_t *msg, on_parse_msg on_msg) {
    ssize_t bytes;
    strbuf_t buf;
    msg_clear(msg);
    strbufalloc(&buf, 256, 256);
    while ((bytes = read(fd, buf.ptr + buf.len, buf.chunk_size)) > 0) {
        buf.len += bytes;
        if (-1 == strbufsize(&buf, buf.len + buf.chunk_size, 0)) {
            msg_clear(msg);
            return -1;
        }
        if (buf.len < *(uint32_t*)buf.ptr)
            continue;
        if (buf.len > *(uint32_t*)buf.ptr) {
            msg_clear(msg);
            return -1;
        } else
            break;
    }
    if (bytes < 0)
        return -1;
    return on_msg(msg, buf.ptr, buf.len);
}

static void unet_save_tail (netbuf_t *nbuf, ssize_t nbytes) {
    if (nbytes < nbuf->buf.len) {
        nbuf->tail.ptr = nbuf->buf.ptr + nbytes;
        nbuf->tail.len = nbuf->buf.len - nbytes;
    } else {
        nbuf->tail.ptr = NULL;
        nbuf->tail.len = 0;
    }
}

int unet_recv (int fd, netbuf_t *nbuf, msgbuf_t *result, msg_parse_h fn_parse) {
    int rc = 0;
    ssize_t nbytes;
    if ((nbytes = msg_buflen(nbuf->buf.ptr, nbuf->buf.len)) > 0) {
        rc = fn_parse(result, nbuf->buf.ptr, nbytes);
        unet_save_tail(nbuf, nbytes);
        if (MSG_OK == rc)
            return NET_OK;
        return NET_WAIT;
    }
    memset(result, 0, sizeof(msgbuf_t));
    if ((nbytes = net_recv(fd, &nbuf->buf)) > 0) {
        if (-1 == (nbytes = msg_buflen(nbuf->buf.ptr, nbuf->buf.len)))
            return NET_WAIT;
        rc = fn_parse(result, nbuf->buf.ptr, nbytes);
        unet_save_tail(nbuf, nbytes);
        if (MSG_OK == rc)
            return NET_OK;
        return NET_WAIT;
    }
    return NET_ERROR;
}


ssize_t unet_write (int fd, char *buf, size_t size) {
    ssize_t sent = 0, wrote;
    while (sent < size) {
        do
            wrote = write(fd, buf, size - sent);
        while (wrote < 0 && errno == EINTR);
        if (wrote <= 0) {
            if (errno != EAGAIN) sent = -1;
            break;
        }
        sent += wrote;
        buf += wrote;
    }
    return sent;
}

void unet_reset (netbuf_t *nbuf) {
    if (nbuf->tail.ptr && nbuf->tail.len) {
        memmove(nbuf->buf.ptr, nbuf->tail.ptr, nbuf->tail.len);
        nbuf->buf.len = nbuf->tail.len;
    } else
        nbuf->buf.len = 0;
}
