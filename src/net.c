#include "net.h"

#define CONN_BUF_SIZE 512

mutex_h fn_lock = NULL;
mutex_h fn_unlock = NULL;

int netbuf_alloc (netbuf_t *nbuf, size_t start_len, size_t chunk_size) {
    if (-1 == strbufalloc(&nbuf->buf, start_len, chunk_size) || -1 == strbufalloc(&nbuf->tail, start_len, chunk_size))
        return -1;
    return 0;
}

int atoport (const char *service, const char *proto) {
    struct servent *servent;
    int port;
    char *tail;
    if ((servent = getservbyname(service, proto)))
        return servent->s_port;
    port = strtol(service, &tail, 0);
    if ('\0' != *tail || port < 1 || port > 65535)
        return -1;
    return htons(port);
}

int atoaddr (const char *address, struct in_addr *addr) {
    struct hostent *host;
    addr->s_addr = inet_addr(address);
    if (-1 == addr->s_addr)
        return -1;
    if (!(host = gethostbyname(address)))
        return -1;
    *addr = *(struct in_addr*)*host->h_addr_list;
    return 0;
}

int net_bind (const char *svc) {
    struct sockaddr_in6 inaddr;
    int fd = socket(AF_INET6, SOCK_STREAM, 0), on = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    memset(&inaddr, 0, sizeof(inaddr));
    inaddr.sin6_family = AF_INET6;
    inaddr.sin6_port = atoport(svc, "tcp");
    inaddr.sin6_addr = in6addr_any;
    if (bind(fd, (struct sockaddr*)&inaddr, sizeof(inaddr)) < 0) {
        close(fd);
        return -1;
    }
    if(listen(fd, SOMAXCONN) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}
#if 0
static int net_wait (int fd, int timeout) {
    int rc;
    struct pollfd fds;
    long nbytes = -1;
    memset(&fds, 0, sizeof(fds));
    fds.fd = fd;
    fds.events = POLLIN;
    if (0 == (rc = ioctl(fd, FIONREAD, &nbytes)) && nbytes > 0)
        return 1;
    if (-1 == rc)
        return -1;
    rc = poll(&fds, 1, timeout);
    return rc;
}

ssize_t net_recv_r (int fd, int timeout, strbuf_t *buf, void *locker, fmt_checker_h fn_check) {
    int done = 0, rc = -1;
    ssize_t total = 0;
    errno = 0;
    while (!done && (rc = net_wait(fd, timeout) > 0)) {
        while (1) {
            ssize_t readed;
            if (-1 == strbufsize(buf, buf->len + buf->chunk_size, 0)) {
                done = 1;
                break;
            }
            if (locker && fn_lock && fn_unlock) {
                fn_lock(locker);
                readed = recv(fd, buf->ptr + buf->len, buf->chunk_size, MSG_DONTWAIT);
                fn_unlock(locker);
            } else
                readed = recv(fd, buf->ptr + buf->len, buf->chunk_size, MSG_DONTWAIT);
            if (-1 == readed) {
                if (EAGAIN == errno)
                    errno = 0;
                done = 1;
                rc = 1;
                break;
            }
            if (0 == readed) {
                long nbytes = -1;
                if (0 == ioctl(fd, FIONREAD, &nbytes) && nbytes > 0)
                    continue;
                rc = -1;
                done = 1;
                break;
            }
            buf->len += readed;
            total += readed;
            if (fn_check && -1 != fn_check(buf->ptr, buf->len)) {
                done = 1;
                break;
            }
        }
    }
    return rc > 0 ? total : rc;
}
#endif

ssize_t net_recv (int fd, strbuf_t *buf, fmt_checker_h fn_check) {
    ssize_t readed;
    if (-1 == strbufsize(buf, buf->len + buf->chunk_size, 0))
        return -1;
    if ((readed = recv(fd, buf->ptr + buf->len, buf->chunk_size, 0)) > 0)
        buf->len += readed;
    return readed;
}


ssize_t net_write (int fd, char *buf, size_t size, void *locker) {
    ssize_t sent = 0, wrote;
    while (sent < size) {
        do {
            if (locker && fn_lock && fn_unlock) {
                fn_lock(locker);
                wrote = write(fd, buf, size - sent);
                fn_unlock(locker);
            } else
                wrote = write(fd, buf, size - sent);
        } while (wrote < 0 && errno == EINTR);
        if (wrote <= 0) {
            if (errno != EAGAIN) sent = -1;
            break;
        }
        sent += wrote;
        buf += wrote;
    }
    return sent;
}

int net_connect (char *to_addr, char *service, int timeout) {
    int port, sock, rc = -1, flags, err;
    socklen_t len = sizeof(err);
    struct in_addr addr;
    struct sockaddr_in in_addr;
    fd_set rd, wr, ex;
    memset(&addr, 0, sizeof(struct in_addr));
    if (-1 == (port = atoport(service, "tcp")) || (-1 == atoaddr(to_addr, &addr)))
        return -1;
    memset(&in_addr, 0, sizeof in_addr);
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = port;
    in_addr.sin_addr.s_addr = addr.s_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (timeout > 0) {
        if (0 > (flags = fcntl(sock, F_GETFL, 0)) ||
            0 > fcntl(sock, F_SETFL, flags | O_NONBLOCK) ||
            ((rc = connect(sock, (struct sockaddr*)&in_addr, sizeof in_addr)) && EINPROGRESS != errno))
            goto err;
    } else
    if (0 == (rc = connect(sock, (struct sockaddr*)&in_addr, sizeof in_addr))) {
        if (0 > (flags = fcntl(sock, F_GETFL, 0)) || 0 > fcntl(sock, F_SETFL, O_NONBLOCK | flags))
            goto err;
        return sock;
    } else
        goto err;
    FD_ZERO(&rd);
    FD_SET(sock, &rd);
    wr = rd = ex;
    long ns = (timeout % 1000) * 1000000;
    struct timeval tv = { .tv_sec = timeout / 1000 + (ns / 1000000000), .tv_usec = ns % 1000000000 };
    if (0 > (rc = select(sock+1, &rd, &wr, &ex, &tv)))
        goto err;
    if (0 == rc)
        goto err;
    errno = 0;
    if (!FD_ISSET(sock, &rd) && !FD_ISSET(sock, &wr))  return 0;
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &err, &len) < 0) return 0;
    errno = err;
    if (0 == err) return sock;
    return sock;
err:
    close(sock);
    return -1;
}
