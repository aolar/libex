#include "net.h"

in_addr_t listen_addr;
net_daemon_t *srv_daemon = NULL;

static int on_try_connect(struct sockaddr_in *in_addr, int fd) {
    return in_addr->sin_addr.s_addr == listen_addr ? NETSRV_OK : NETSRV_DONE;
}

static void on_connect (int fd, netconn_t *conn) {
    printf("connected: %d\n", fd);
}

static int on_disconnect (int fd, netconn_t *data) {
    printf("disconnect: %d\n", fd);
    return 0;
}

static int on_event (int fd, strbuf_t *buf, netconn_t *conn) {
    if (0 == cmpstr(buf->ptr, buf->len, CONST_STR_LEN("close")))
        return NETSRV_DONE;
    char *str = strndup(buf->ptr, buf->len), resp [256], *tail;
    int n = strtol(str, &tail, 0);
    if ('\0' == *tail && ERANGE != errno) {
        snprintf(resp, sizeof(resp), "int: %d", n);
        printf("%s\n", resp);
        goto done;
    }
    double d = strtod(str, &tail);
    if (ERANGE != errno) {
        snprintf(resp, sizeof(resp), "double: %f", d);
        printf("%s\n", resp);
        goto done;
    }
    snprintf(resp, sizeof(resp), "illegal number %s", str);
    printf("%s\n", resp);
done:
    net_write(fd, resp, strlen(resp));
    free(str);
    return NETSRV_OK;
}

static void signal_handler (int sig) {
    if (srv_daemon) {
        netsrv_close(srv_daemon);
        srv_daemon = NULL;
    }
    exit(0);
}

static void setsig () {
    struct sigaction sig;
    sig.sa_handler = signal_handler;
    sig.sa_flags = 0;
    sigemptyset(&sig.sa_mask);
    sigaction(SIGTERM, &sig, NULL);
    sigaction(SIGINT, &sig, NULL);
}

int main () {
    srv_daemon = netsrv_init();
    listen_addr = inet_addr("127.0.0.1");
    netsrv_setopt(srv_daemon, NETSRV_THREADS, 0);
    netsrv_setopt(srv_daemon, NETSRV_SERVICE, "9876");
    netsrv_setopt(srv_daemon, NETSRV_TRYCONNECT, on_try_connect);
    netsrv_setopt(srv_daemon, NETSRV_CONNECT, on_connect);
    netsrv_setopt(srv_daemon, NETSRV_EVENT, on_event);
    netsrv_setopt(srv_daemon, NETSRV_DISCONNECT, on_disconnect);
    setsig();
    printf("[%d]\n", getpid());
    netsrv_start(srv_daemon);
    return 0;
}
