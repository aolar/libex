/**
 * @file net.h
 * @brief network functions
 */
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

/** current network operation is successed, can be anything */
#define NETSRV_OK 0
/** current network operation is done */
#define NETSRV_DONE 1
/** wait status */
#define NETSRV_WAIT 2

/** @brief net service flags */
typedef enum {
    /** thread count */
    NETSRV_THREADS,
    /** port or service name */
    NETSRV_SERVICE,
    /** callback function before connecting */
    NETSRV_TRYCONNECT,
    /** callback function after connecting */
    NETSRV_CONNECT,
    /** callback function after receive data */
    NETSRV_EVENT,
    /** callback function after disconnect */
    NETSRV_DISCONNECT
} netsrv_opt_t;

/** @brief callback function for #NETSRV_TRYCONNECT
 * @param 1-st sockaddr_in
 * @param 2-nd socket
 */
typedef int (*srv_try_connect_h) (struct sockaddr_in*, int);
/** callback function for #NETSRV_CONNECT
 * @param 1-st socket
 */
typedef void* (*srv_connect_h) (int);
/** @brief function for #NETSRV_EVENT
 * @param 1-st buffer with received data
 * @param 2-nd user data
 */
typedef int (*srv_event_h) (int, strbuf_t *buf, void *data);
/** @brief function for #NETSRV_DISCONNECT
 * @param 1-st socket
 * @param 2-nd user data
 */
typedef int (*srv_disconnect_h) (int, void*);
/** @brief opaque net service structure */
typedef struct net_daemon net_daemon_t;

/** @brief net service thread info */
typedef struct {
    /** is main thread */
    int is_main;
    /** process identifier */
    pthread_t pid;
    /** connection count */
    size_t conn_count;
    /** waited connection count */
    size_t waited_conn_count;
} netsrv_info_t;

/** @brief daemon structure */
typedef struct {
    /** thread count */
    int thread_count;
    /** structure for each thread */
    netsrv_info_t srv_info [0];
} net_daemon_info_t;

/** @brief initialize daemon
 * @retval net daemon structure
 */
net_daemon_t *netsrv_init();

/** @brief set integer parameter for daemon
 * @param daemon
 * @param opt option
 * @param x paramemter
 */
extern void netsrv_setopt_int (net_daemon_t *daemon, netsrv_opt_t opt, int x);

/** @brief set character parameter for daemon
 * @param daemon
 * @param opt option
 * @param x paramemter
 */
extern void netsrv_setopt_str (net_daemon_t *daemon, netsrv_opt_t opt, const char *x);

/** @brief set integer callback function for daemon
 * @param daemon
 * @param opt option
 * @param x paramemter
 */
extern void netsrv_setopt_try_connect (net_daemon_t *daemon, netsrv_opt_t opt, srv_try_connect_h x);

/** @brief set integer callback function for daemon
 * @param daemon
 * @param opt option
 * @param x paramemter
 */
extern void netsrv_setopt_connect (net_daemon_t *daemon, netsrv_opt_t opt, srv_connect_h x);

/** @brief set integer callback function for daemon
 * @param daemon
 * @param opt option
 * @param x paramemter
 */
extern void netsrv_setopt_event (net_daemon_t *daemon, netsrv_opt_t opt, srv_event_h x);

/** @brief set integer callback function for daemon
 * @param daemon
 * @param opt option
 * @param x paramemter
 */
extern void netsrv_setopt_disconnect (net_daemon_t *daemon, netsrv_opt_t opt, srv_disconnect_h x);

/** @brief generic for \b netsrv_setopt function */
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

/** @brief start daemon
 * @param daemon
 * @retval 0 if success, -1 if error
 */
int netsrv_start (net_daemon_t *daemon);

/** @brief free daemon
 * @param daemon
 */
void netsrv_close (net_daemon_t *daemon);

/** @brief get daemon info
 * @param daemon
 * @retval daemon information
 */
net_daemon_info_t *get_daemon_info (net_daemon_t *daemon);

/** @brief client function for open connection
 * @param to_addr address or daemon
 * @param service or port number
 * @param timeout timeout for connection, if timeout <= 0 then it is infinity
 * @retval socket
 */
int net_connect (char *to_addr, char *service, int timeout);

/** @brief read data from socket
 * @param fd socket
 * @param buf buffer for read data
 * @param timeout timeout for reading
 * @retval count of read data
 */
ssize_t net_read (int fd, strbuf_t *buf, int timeout);

/** @brief write data to socket
 * @param fd socket
 * @param buf buffer
 * @param size size of buffer
 * @retval count of wrote data
 */
ssize_t net_write (int fd, char *buf, size_t size);

#endif // __LIBEX_NET_H__
