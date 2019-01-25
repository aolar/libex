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
#include <poll.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/resource.h>
#include "str.h"
#include "list.h"
#include "task.h"

#define NET_OK 1
#define NET_WAIT 0
#define NET_ERROR -1

typedef int (*mutex_h) (void*);
extern mutex_h fn_lock;
extern mutex_h fn_unlock;

typedef struct {
    strbuf_t buf;
    strbuf_t tail;
} netbuf_t;
int netbuf_alloc (netbuf_t *nbuf, size_t start_len, size_t chunk_size);
void netbuf_free (netbuf_t *nbuf);

typedef ssize_t (*fmt_checker_h) (const char*, size_t);

int atoport (const char *service, const char *proto);
int atoaddr (const char *address, struct in_addr *addr);

typedef ssize_t (*net_recv_h) (int, strbuf_t*);

int net_bind (const char *svc);
int net_connect (char *to_addr, char *service, int timeout);
ssize_t net_recv (int fd, strbuf_t *buf);
ssize_t net_recvnb (int fd, strbuf_t *buf);
ssize_t net_write (int fd, char *buf, size_t size, void *locker);

#endif // __LIBEX_NET_H__
