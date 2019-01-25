#ifndef __LIBEX_WSNET_H__
#define __LIBEX_WSNET_H__

#include <sys/time.h>
#include "list.h"
#include "net.h"
#include "ws.h"

#define WST_PING_LEN 7
#define WST_CLOSE_LEN 6

extern char wst_ping [];
extern char wst_close [];

int wsnet_handshake (int fd);
int wsnet_try_handshake (int fd, strbuf_t *buf);
int wsnet_recvfn (int fd, netbuf_t *nbuf, net_recv_h on_recv, ws_t **result);
static inline int wsnet_recv (int fd, netbuf_t *nbuf, ws_t **result) {
    return wsnet_recvfn(fd, nbuf, net_recv, result);
}
static inline int wsnet_recvnb (int fd, netbuf_t *nbuf, ws_t **result) {
    return wsnet_recvfn(fd, nbuf, net_recvnb, result);
}
void wsnet_reset (netbuf_t *nbuf);

#endif // __LIBEX_WSNET_H__
