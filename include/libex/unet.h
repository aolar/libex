#ifndef __LIBEX_UNET_H__
#define __LIBEX_UNET_H__

#include <stdint.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "str.h"
#include "msg.h"
#include "net.h"

typedef int (*on_parse_msg) (msgbuf_t*, char*, size_t);

int unet_bind (const char *sock_file);
int unet_connect (const char *sock_file);
int unet_read (int fd, msgbuf_t *msg, on_parse_msg on_msg);
static inline int unet_read_request (int fd, msgbuf_t *msg) { return unet_read(fd, msg, msg_load_request); }
static inline int unet_read_response (int fd, msgbuf_t *msg) { return unet_read(fd, msg, msg_load_response); }
ssize_t unet_write (int fd, char *buf, size_t size);
int unet_recv (int fd, netbuf_t *nbuf, msgbuf_t *result, msg_parse_h on_parse);
static inline int unet_recv_request(int fd, netbuf_t *buf, msgbuf_t *result) { return unet_recv(fd, buf, result, msg_load_request); }
static inline int unet_recv_response (int fd, netbuf_t *buf, msgbuf_t *result) { return unet_recv(fd, buf, result, msg_load_response); }
void unet_reset (netbuf_t *nbuf);

#endif // __LIBEX_UNET_H__
