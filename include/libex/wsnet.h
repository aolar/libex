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
int wsnet_recv (int fd, int timeout, netbuf_t *buf, void *locker, ws_t **result);
void wsnet_reset (netbuf_t *nbuf);

#endif // __LIBEX_WSNET_H__
