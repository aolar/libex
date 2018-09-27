#include "net.h"

#define CONN_BUF_SIZE 512

static int atoport (const char *service, const char *proto) {
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

static int atoaddr (const char *address, struct in_addr *addr) {
    struct hostent *host;
    addr->s_addr = inet_addr(address);
    if (-1 == addr->s_addr)
        return -1;
    if (!(host = gethostbyname(address)))
        return -1;
    *addr = *(struct in_addr*)*host->h_addr_list;
    return 0;
}

static void on_netconn_free (netconn_t *conn, void *dummy) {
    if (conn->fd > 0)
        close(conn->fd);
    free(conn->buf.ptr);
    free(conn);
}

static int netsrv_srv_init (netsrv_t *srv, int is_main) {
    srv->sfd = srv->efd = 0;
    if (is_main) {
        int port = atoport(srv->daemon->service, "tcp"), flags;
        if (-1 == port)
            goto err;
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(struct sockaddr_in));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_ANY);
        addr.sin_port = port;
        if (-1 == (srv->status = (srv->sfd = socket(AF_INET, SOCK_STREAM, 0))) ||
            -1 == (srv->status = setsockopt(srv->sfd, SOL_SOCKET, SO_REUSEADDR, &(int){1}, sizeof(int))) ||
            -1 == (srv->status = bind(srv->sfd, (struct sockaddr*)&addr, sizeof(struct sockaddr_in))) ||
            -1 == (srv->status = (flags = fcntl(srv->sfd, F_GETFL, 0))) ||
            -1 == (srv->status = fcntl(srv->sfd, F_SETFL, flags | O_NONBLOCK)) ||
            -1 == (srv->status = listen(srv->sfd, SOMAXCONN)))
        goto err;
    }
    if (-1 == (srv->status = (srv->efd = epoll_create1(0))))
        goto err;
    if (is_main) {
        srv->conn.fd = srv->sfd;
        srv->event.data.ptr = &srv->conn;
        srv->event.events = EPOLLIN | EPOLLET;
        if (-1 == (srv->status = epoll_ctl(srv->efd, EPOLL_CTL_ADD, srv->sfd, &srv->event)))
            goto err;
    }
    srv->is_active = 1;
    srv->conns = lst_alloc((free_h)on_netconn_free);
    srv->waited_conns = lst_alloc(NULL);
    return 0;
err:
    if (srv->efd > 0) {
        close(srv->efd);
        srv->efd = 0;
    }
    if (srv->sfd > 0) {
        close(srv->sfd);
        srv->sfd = 0;
    }
    return -1;
}

static netconn_t *netconn_create (int fd, struct sockaddr *in_addr) {
    netconn_t *conn = calloc(1, sizeof(netconn_t));
    conn->fd = fd;
    memcpy(&conn->addr, &((struct sockaddr_in*)in_addr)->sin_addr, sizeof(struct in_addr));
    strbufalloc(&conn->buf, CONN_BUF_SIZE, CONN_BUF_SIZE);
    return conn;
}

static void netconn_free (netconn_t *conn) {
    netsrv_t *srv = conn->srv;
    net_daemon_t *daemon = srv->daemon;
    if (daemon->on_disconnect)
        daemon->on_disconnect(conn->fd, conn);
    pthread_rwlock_wrlock(&srv->locker);
    if (conn->li_wait)
        lst_del(conn->li_wait);
    lst_del(conn->li);
    pthread_rwlock_unlock(&srv->locker);
}

static netsrv_t *select_netsrv (net_daemon_t *daemon) {
    netsrv_t *srvs = daemon->srvs, *srv;
    size_t m = srvs->conns->len;
    if (0 == m)
        return srvs;
    srv = srvs++;
    for (int i = 1; i < daemon->max_threads; ++i) {
        size_t l = srvs->conns->len;
        if (l < m) {
            srv = srvs;
            m = l;
        }
        srvs++;
    }
    return srv;
}

static int netconn_new (netsrv_t *srv) {
    struct sockaddr in_addr;
    socklen_t in_len = sizeof in_addr;
    int flags, in_fd;
    netconn_t *conn = NULL;
    net_daemon_t *daemon = srv->daemon;
    struct epoll_event event;
    if (-1 == (in_fd = accept(srv->sfd, &in_addr, &in_len)) ||
        -1 == (flags = fcntl(in_fd, F_GETFL, 0)) ||
        -1 == fcntl(in_fd, F_SETFL, flags | O_NONBLOCK) ||
        (daemon->on_try_connect && NETSRV_DONE == daemon->on_try_connect((struct sockaddr_in*)&in_addr, in_fd)))
            goto err;
    event.data.ptr = conn = netconn_create(in_fd, &in_addr);
    event.events = EPOLLIN | EPOLLET;
    netsrv_t *srv_io = select_netsrv(daemon);  //conn->srv = &daemon->srvs[daemon->srv_id == daemon->max_threads - 1 ? 0 : daemon->srv_id++];
    pthread_rwlock_wrlock(&srv_io->locker);
    conn->li = lst_adde(srv_io->conns, conn);
    pthread_rwlock_unlock(&srv_io->locker);
    if (-1 == epoll_ctl(srv_io->efd, EPOLL_CTL_ADD, in_fd, &event))
        goto err;
    if (daemon->on_connect)
        daemon->on_connect(conn->fd, conn);
    return 0;
err:
    if (in_fd > 0)
        close(in_fd);
    if (conn)
        netconn_free(conn);
    return -1;
}

static void netconn_io (netsrv_t *srv, netconn_t *conn) {
    int done = 0;
    net_daemon_t *daemon = srv->daemon;
    while (1) {
        ssize_t readed;
        errno = 0;
        strbufsize(&conn->buf, conn->buf.len + conn->buf.chunk_size, 0);
        if (-1 == (readed = recv(conn->fd, conn->buf.ptr + conn->buf.len, conn->buf.chunk_size, 0))) {
            if (EAGAIN != errno)
                done = 1;
            break;
        } else
        if (0 == readed) {
            done = 1;
            break;
        }
        conn->buf.len += readed;
    }
    switch (daemon->on_event(conn->fd, &conn->buf, conn)) {
        case NETSRV_DONE:
            pthread_rwlock_wrlock(&srv->locker);
            if (conn->li_wait) {
                lst_del(conn->li_wait);
                conn->li_wait = NULL;
            }
            pthread_rwlock_unlock(&srv->locker);
            done = 1;
            break;
        case NETSRV_WAIT:
            pthread_rwlock_wrlock(&srv->locker);
            if (!conn->li_wait)
                conn->li_wait = lst_adde(srv->waited_conns, conn);
            pthread_rwlock_unlock(&srv->locker);
            break;
        case NETSRV_OK:
            pthread_rwlock_wrlock(&srv->locker);
            if (conn->li_wait) {
                lst_del(conn->li_wait);
                conn->li_wait = NULL;
            }
            pthread_rwlock_unlock(&srv->locker);
            break;
    }
    strbufsize(&conn->buf, conn->buf.chunk_size, STR_REDUCE);
    conn->buf.len = 0;
    if (done)
        netconn_free(conn);
}

static netconn_t *get_next_conn (netsrv_t *srv) {
    netconn_t *conn = NULL;
    pthread_rwlock_rdlock(&srv->locker);
    list_item_t *li = srv->waited_conns->head;
    if (li) {
        conn = (netconn_t*)li->ptr;
        srv->waited_conns->head = li->next;
    }
    pthread_rwlock_unlock(&srv->locker);
    return conn;
}

static int netsrv_wait (netsrv_t *srv) {
    netconn_t *conn;
    if ((conn = get_next_conn(srv))) {
        net_daemon_t *daemon = srv->daemon;
        int timeout = 0;
        switch (daemon->on_event(conn->fd, &conn->buf, conn)) {
            case NETSRV_DONE:
                netconn_free(conn);
                pthread_rwlock_rdlock(&srv->locker);
                if (0 == srv->waited_conns->len)
                    timeout = -1;
                pthread_rwlock_unlock(&srv->locker);
                break;
            case NETSRV_WAIT:
                break;
            case NETSRV_OK:
                pthread_rwlock_wrlock(&srv->locker);
                lst_del(conn->li_wait);
                conn->li_wait = NULL;
                if (0 == srv->waited_conns->len)
                    timeout = -1;
                pthread_rwlock_unlock(&srv->locker);
                break;
        }
        return epoll_wait(srv->efd, srv->events, srv->max_events, timeout);
    }
    return epoll_wait(srv->efd, srv->events, srv->max_events, srv->daemon->ms_timeout);
}

static void *some_netsrv_start (netsrv_t *srv) {
    net_daemon_t *daemon = srv->daemon;
    int is_main = srv == &daemon->srvs[0];
    srv->max_events = SOMAXCONN;
    struct epoll_event *events = srv->events = calloc(srv->max_events, sizeof(struct epoll_event));
    if (-1 == (srv->status = netsrv_srv_init(srv, is_main))) {
        pthread_barrier_wait(&daemon->barrier);
        return NULL;
    }
    pthread_rwlock_init(&srv->locker, NULL);
    pthread_barrier_wait(&daemon->barrier);
    while (srv->is_active) {
        int n = netsrv_wait(srv);
        for (int i = 0; i < n; ++i) {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP) || (!(events[i].events & EPOLLIN))) {
                netconn_free(events[i].data.ptr);
            } else
            if (is_main && srv->sfd == ((netconn_t*)events[i].data.ptr)->fd)
                netconn_new(srv);
            else
                netconn_io(srv, (netconn_t*)events[i].data.ptr);
        }
    }
    return NULL;
}

static void some_netsrv_close (netsrv_t *srv) {
    srv->is_active = 0;
    if (srv->sfd) {
        epoll_ctl(srv->efd, EPOLL_CTL_DEL, srv->sfd, &srv->event);
        shutdown(srv->sfd, SHUT_RDWR);
        srv->sfd = 0;
    }
    if (srv->efd) {
        close(srv->efd);
        srv->efd = 0;
    }
    if (srv->conns) {
        lst_free(srv->conns);
        srv->conns = NULL;
    }
    if (srv->waited_conns) {
        lst_free(srv->waited_conns);
        srv->waited_conns = NULL;
    }
    if (srv->events) {
        free(srv->events);
        srv->events = NULL;
    }
    pthread_rwlock_destroy(&srv->locker);
    if (srv->pid != pthread_self())
        kill(getpid(), SIGTERM);
}

int netsrv_start (net_daemon_t *daemon) {
    if (!daemon->service || !daemon->on_event)
        return -1;
    if (daemon->max_threads <= 0)
        daemon->max_threads = sysconf(_SC_NPROCESSORS_ONLN);
    daemon->srvs = calloc(daemon->max_threads, sizeof(netsrv_t));
    pthread_barrier_init(&daemon->barrier, NULL, daemon->max_threads);
    for (int i = 1; i < daemon->max_threads; ++i) {
        daemon->srvs[i].daemon = daemon;
        pthread_create(&daemon->srvs[i].pid, NULL, (void*(*)(void*))some_netsrv_start, &daemon->srvs[i]);
    }
    daemon->srvs[0].pid = pthread_self();
    daemon->srvs[0].daemon = daemon;
    some_netsrv_start(daemon->srvs);
    return 0;
}

void netsrv_close (net_daemon_t *daemon) {
    for (int i = 1; i < daemon->max_threads; ++i)
        some_netsrv_close(&daemon->srvs[i]);
    for (int i = 1; i < daemon->max_threads; ++i)
        pthread_join(daemon->srvs[i].pid, NULL);
    some_netsrv_close(&daemon->srvs[0]);
    if (daemon->service)
        free(daemon->service);
    if (daemon->srvs)
        free(daemon->srvs);
    free(daemon);
}

net_daemon_info_t *get_daemon_info (net_daemon_t *daemon) {
    net_daemon_info_t *info = malloc(sizeof(int) + sizeof(netsrv_info_t) * daemon->max_threads);
    info->thread_count = daemon->max_threads;
    for (int i = 0; i < daemon->max_threads; ++i) {
        info->srv_info[i].is_main = daemon->srvs[i].pid == pthread_self() ? 1 : 0;
        info->srv_info[i].pid = daemon->srvs[i].pid;
        pthread_rwlock_rdlock(&daemon->srvs[i].locker);
        info->srv_info[i].conn_count = daemon->srvs[i].conns->len;
        info->srv_info[i].waited_conn_count = daemon->srvs[i].waited_conns->len;
        pthread_rwlock_unlock(&daemon->srvs[i].locker);
    }
    return info;
}

net_daemon_t *netsrv_init () {
    net_daemon_t *daemon = calloc(1, sizeof(net_daemon_t));
    daemon->ms_timeout = -1;
    return daemon;
}

inline void netsrv_setopt_int (net_daemon_t *daemon, netsrv_opt_t opt, int x) {
    switch (opt) {
        case NETSRV_THREADS: daemon->max_threads = x; break;
        case NETSRV_MSTIMEOUT: daemon->ms_timeout = x; break;
        default: break;
    }
}

inline void netsrv_setopt_str (net_daemon_t *daemon, netsrv_opt_t opt, const char *x) {
    daemon->service = strdup(x);
}

inline void netsrv_setopt_try_connect (net_daemon_t *daemon, netsrv_opt_t opt, srv_try_connect_h x) {
    daemon->on_try_connect = x;
}

inline void netsrv_setopt_connect (net_daemon_t *daemon, netsrv_opt_t opt, srv_connect_h x) {
    daemon->on_connect = x;
}

inline void netsrv_setopt_event (net_daemon_t *daemon, netsrv_opt_t opt, srv_event_h x) {
    daemon->on_event = x;
}

inline void netsrv_setopt_disconnect (net_daemon_t *daemon, netsrv_opt_t opt, srv_disconnect_h x) {
    daemon->on_disconnect = x;
}

static int wait_data (int fd, int timeout) {
    int rc;
    int32_t nbytes = -1;
    fd_set rd, ex;
    if (0 == (rc = ioctl(fd, FIONREAD, &nbytes)) && nbytes > 0)
        return 1;
    if (-1 == rc)
        return -1;
    FD_ZERO(&rd);
    FD_ZERO(&ex);
    FD_SET(fd, &rd);
    FD_SET(fd, &ex);
    if (timeout >= 0) {
        struct timeval tv = { .tv_sec = timeout / 1000, .tv_usec = (timeout % 1000) * 1000 };
        rc = select(fd+1, &rd, NULL, &ex, &tv);
    } else
        rc = select(fd+1, &rd, NULL, &ex, NULL);
    if (rc > 0) {
        if (FD_ISSET(fd, &ex)) {
            errno = EINTR;
            return -1;
        }
        if (FD_ISSET(fd, &rd))
            return 1;
    }
    return 0;
}

ssize_t net_read (int fd, strbuf_t *buf, int timeout) {
    int done = 0;
    ssize_t total = 0;
    errno = 0;
    buf->len = 0;
    while (!done && wait_data(fd, timeout) > 0) {
        while (1) {
            strbufsize(buf, buf->len + buf->chunk_size, 0);
            ssize_t readed = recv(fd, buf->ptr + buf->len, buf->chunk_size, 0);
            if (-1 == readed) {
                if (EAGAIN == errno)
                    errno = 0;
                done = 1;
                break;
            } else
            if (0 == readed) {
                long nbytes;
                if (0 == ioctl(fd, FIONREAD, &nbytes) && nbytes > 0)
                    continue;
                done = 1;
                break;
            }
            buf->len += readed;
            total += readed;
        }
    }
    return total;
}

ssize_t net_write (int fd, char *buf, size_t size) {
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
