#ifndef __LIBEX_NET_H__
#define __LIBEX_NET_H__

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/resource.h>
#include "str.h"
#include "list.h"
#include "task.h"

#define NETSRV_OK 0
#define NETSRV_DONE 1
#define NETSRV_WAIT 2

typedef enum { NETSRV_THREADS, NETSRV_SERVICE, NETSRV_TRYCONNECT, NETSRV_CONNECT, NETSRV_EVENT, NETSRV_DISCONNECT } netsrv_opt_t;
typedef int (*srv_try_connect_h) (struct sockaddr_in*, int);
typedef void* (*srv_connect_h) (int);
typedef int (*srv_event_h) (int, strbuf_t *buf, void *data);
typedef int (*srv_disconnect_h) (int, void*);
typedef struct net_daemon net_daemon_t;

typedef struct {
    int is_main;
    pthread_t pid;
    size_t conn_count;
    size_t waited_conn_count;
} netsrv_info_t;

typedef struct {
    int thread_count;
    netsrv_info_t srv_info [0];
} net_daemon_info_t;

net_daemon_t *netsrv_init();
extern void netsrv_setopt_int (net_daemon_t *daemon, netsrv_opt_t opt, int x);
extern void netsrv_setopt_str (net_daemon_t *daemon, netsrv_opt_t opt, const char *x);
extern void netsrv_setopt_try_connect (net_daemon_t *daemon, netsrv_opt_t opt, srv_try_connect_h x);
extern void netsrv_setopt_connect (net_daemon_t *daemon, netsrv_opt_t opt, srv_connect_h x);
extern void netsrv_setopt_event (net_daemon_t *daemon, netsrv_opt_t opt, srv_event_h x);
extern void netsrv_setopt_disconnect (net_daemon_t *daemon, netsrv_opt_t opt, srv_disconnect_h x);
#define netsrv_setopt(daemon,opt,x) \
    _Generic((x), \
    int: netsrv_setopt_int, \
    char*: netsrv_setopt_str, \
    srv_try_connect_h: netsrv_setopt_try_connect, \
    srv_connect_h: netsrv_setopt_connect, \
    srv_event_h: netsrv_setopt_event, \
    srv_disconnect_h: netsrv_setopt_disconnect, \
    default: netsrv_setopt_int \
)(daemon,opt,x)

int netsrv_start (net_daemon_t *daemon);
void netsrv_close (net_daemon_t *daemon);
net_daemon_info_t *get_daemon_info (net_daemon_t *daemon);
int net_connect (char *to_addr, char *service, int timeout);
ssize_t net_read (int fd, strbuf_t *buf, int timeout);
ssize_t net_write (int fd, char *buf, size_t size);

#endif // __LIBEX_NET_H__
