#ifndef __LIBEX_TASK_H__
#define __LIBEX_TASK_H__

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/time.h>
#ifndef __WIN32__
#include <sys/wait.h>
#include <signal.h>
#else
#include <windows.h>
#include <tchar.h>
#endif
#include "str.h"
#include "list.h"

typedef enum {
    RUN_EXITED,
    RUN_STOPPED,
    RUN_SIGNALED
} run_exit_t;

#define RUN_WAIT 1

typedef struct {
    #ifndef __WIN32__
    pid_t pid;
    int exit_code;
    #else
    PROCESS_INFORMATION pi;
    DWORD exit_code;
    #endif
    strptr_t cmd;
    run_exit_t run_exit;
} run_t;

char **mkcmdstr (const char *cmdline, size_t cmdline_len);
void free_cmd (char **cmd);
void run (const char *cmd, size_t cmd_len, run_t *proc, int flags);

#define POOL_DEFAULT_SLOTS -1

#define MSG_DONE 0
#define MSG_CONTINUE 1

typedef struct slot slot_t;
typedef int (*pool_msg_h) (void *slot_data, void *in_data, void *out_data);
typedef int (*pool_create_h) (slot_t*, void*);
typedef void (*pool_destroy_h) (slot_t*);

typedef enum {
    POOL_MSG,
    POOL_FREEMSG,
    POOL_MAXSLOTS,
    POOL_TIMEOUT,
    POOL_LIVINGTIME,
    POOL_CREATESLOT,
    POOL_DESTROYSLOT
} pool_opt_t;

typedef struct {
    void *in_data;
    void *out_data;
    pool_msg_h on_msg;
    pthread_mutex_t *mutex;
    pthread_cond_t *cond;
} msg_t;

typedef struct {
    int is_alive;
    pthread_mutex_t locker;
    pthread_condattr_t cond_attr;
    pthread_cond_t cond;
    list_t *queue;
    long max_slots;
    list_t *slots;
    pool_msg_h on_msg;
    pool_destroy_h on_freemsg;
    pool_create_h on_create_slot;
    pool_destroy_h on_destroy_slot;
    pthread_t th;
    long timeout;
    long livingtime;
} pool_t;

struct slot {
    int is_alive;
    pool_t *pool;
    pthread_t th;
    list_item_t *node;
    void *data;
};

int pool_setopt_int (pool_t *pool, pool_opt_t opt, long arg);
int pool_setopt_msg (pool_t *pool, pool_opt_t opt, pool_msg_h arg);
int pool_setopt_create (pool_t *pool, pool_opt_t opt, pool_create_h arg);
int pool_setopt_destroy (pool_t *pool, pool_opt_t opt, pool_destroy_h arg);

#define pool_setopt(pool,opt,arg) \
    _Generic((arg), \
    pool_msg_h: pool_setopt_msg, \
    pool_create_h: pool_setopt_create, \
    pool_destroy_h: pool_setopt_destroy, \
    default: pool_setopt_int \
)(pool,opt,arg)

msg_t *pool_createmsg (pool_msg_h on_msg, void *in_data, void *out_data, pthread_mutex_t *mutex, pthread_cond_t *cond);

pool_t *pool_create ();

void pool_start (pool_t *pool);
int pool_call (pool_t *pool, msg_t *msg, void *init_data);
int pool_callmsgwait (pool_t *pool, pool_msg_h on_msg, void *in_data, void *out_data, void *init_data);
void pool_destroy (pool_t *pool);

#endif // __LIBEX_TASK_h__
