#ifndef __LIBEX_TASK_H__
#define __LIBEX_TASK_H__

#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <signal.h>
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
#include "thread.h"
#include "list.h"

typedef enum { RUN_EXITED, RUN_STOPPED, RUN_SIGNALED } run_exit_t;

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

typedef enum { POOL_MSG, POOL_MAXSLOTS, POOL_TIMEOUT, POOL_LIVINGTIME, POOL_CREATESLOT, POOL_DESTROYSLOT, POOL_INITDATA } pool_opt_t;

typedef struct slot slot_t;
typedef void (*msg_h) (void*);

typedef struct {
    void *data;
    msg_h on_msg;
} msg_t;

typedef struct {
    int is_alive;
    pthread_mutex_t locker;
    pthread_condattr_t cond_attr;
    pthread_cond_t cond;
    list_t *queue;
    long max_slots;
    list_t *slots;
    msg_h on_msg;
    msg_h on_create_slot;
    msg_h on_destroy_slot;
    pthread_t th;
    long timeout;
    long livingtime;
    void *init_data;
} pool_t;

struct slot {
    int is_alive;
    pool_t *pool;
    pthread_t th;
    list_item_t *node;
    void *data;
};

int pool_setopt_int (pool_t *pool, pool_opt_t opt, long arg);
int pool_setopt_msg (pool_t *pool, pool_opt_t opt, msg_h arg);
int pool_setopt_void (pool_t *pool, pool_opt_t opt, void *arg);
#define pool_setopt(pool,opt,arg) \
    _Generic((arg), \
    msg_h: pool_setopt_msg, \
    void*: pool_setopt_void, \
    default: pool_setopt_int \
)(pool,opt,arg)

pool_t *pool_create ();
void pool_start (pool_t *pool);
void pool_call (pool_t *pool, msg_h on_msg, void *data);
void pool_destroy (pool_t *pool);

#endif // __LIBEX_TASK_h__
