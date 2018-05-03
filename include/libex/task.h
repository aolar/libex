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

#define MSG_MUSTFREE 0x00000001

#define TASK_DEFAULT_SLOTS -1

typedef enum { TASK_MSG, TASK_FREEMSG, TASK_MAXSLOTS, TASK_TIMEOUT } task_opt_t;

typedef struct slot slot_t;
typedef void (*msg_h) (void*);
typedef void (*msgfree_h) (void*);

typedef struct {
    int flags;
    void *req;
} msg_t;

typedef struct {
    int is_alive;
    pthread_mutex_t locker;
    pthread_cond_t cond;
    list_t *queue;
    long max_slots;
    list_t *slots;
    msg_h on_msg;
    msgfree_h on_freemsg;
    pthread_t th;
    long timeout;
} task_t;

struct slot {
    int is_alive;
    task_t *task;
    pthread_t th;
};

int task_setopt_msg (task_t *task, task_opt_t, msg_h arg);
int task_setopt_msgfree (task_t *task, task_opt_t opt, msgfree_h arg);
int task_setopt_int (task_t *task, task_opt_t opt, long arg);
#define task_setopt(task,opt,arg) \
    _Generic((arg), \
    int: task_setopt_int, \
    msg_h: task_setopt_msg, \
    default: task_setopt_msgfree \
)(task,opt,arg)

task_t *task_create ();
void task_start (task_t *task);
void task_cast (task_t *task, void *data, int flags);
void task_destroy (task_t *task);

#endif // __LIBEX_TASK_h__
