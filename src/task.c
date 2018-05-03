#include "task.h"

#if 0
char **mkcmdstr (const char *cmdline, size_t cmd_len) {
    str_t *cmd_str = mkstr(cmdline, cmd_len, 8);
    strptr_t cmd = { cmd_str->len, cmd_str->ptr }, tok;
    size_t cnt = 0, n = 1;
    char **args = NULL;
    STR_ADD_NULL(cmd_str);
    while (0 == strntok(&cmd.ptr, &cmd.len, CONST_STR_LEN(" "), &tok)) {
        if (n) {
            if (tok.ptr[0] == '"') n = 0;
            if (cnt > 0) *(tok.ptr - n) = '\0';
            ++cnt;
        } else
        if (tok.ptr[tok.len-1] == '"') {
            *(tok.ptr + tok.len - 1) = '\0';
            n = 1;
        }
    }
    cmd.ptr = cmd_str->ptr;
    cmd.len = cmd_str->len;
    if (0 == strntok(&cmd.ptr, &cmd.len, CONST_STR_LEN("\0"), &tok)) {
        char *p;
        args = malloc(sizeof(char*) * (cnt + 2));
        args[0] = malloc(tok.len+1);
        strncpy(args[0], tok.ptr, tok.len);
        args[0][tok.len] = '\0';
        if ((p = strrchr(tok.ptr, '/'))) {
            *p++ = '\0';
            tok.len -= (uintptr_t)p - (uintptr_t)tok.ptr;
            tok.ptr = p;
        }
        args[1] = malloc(tok.len+1);
        strncpy(args[1], tok.ptr, tok.len);
        args[1][tok.len] = '\0';
        n = 2;
        while (0 == strntok(&cmd.ptr, &cmd.len, CONST_STR_LEN("\0"), &tok)) {
            args[n] = malloc(tok.len+1);
            strncpy(args[n], tok.ptr, tok.len);
            args[n][tok.len] = '\0';
            ++n;
        }
        args[n] = NULL;
    }
    free(cmd_str);
    return args;
}
#endif

typedef struct {
    char **cmd;
    size_t idx;
} make_cmd_t;

static int on_make_cmd (list_item_t *item, make_cmd_t *x) {
    x->cmd[x->idx] = (char*)item->ptr;
    ++x->idx;
    return ENUM_CONTINUE;
}

char **mkcmdstr (const char *cmdline, size_t cmdline_len) {
    list_t *lst = lst_alloc(NULL);
    const char *p = cmdline, *e = p + cmdline_len;
    while (p < e) {
        const char *q;
        char c = '\0';
        while (isspace(*p)) ++p;
        q = p;
        if ('"' == *p || '\'' == *p)
            c = *p++;
        if ('\0' == c)
            while (p < e && !isspace(*p)) ++p;
        else {
            while (p < e && *p != c) ++p;
            if (*p == c) ++p;
        }
        char *str = strndup(q, (uintptr_t)p - (uintptr_t)q);
        lst_adde(lst, str);
    }
    make_cmd_t x = { .cmd = calloc(lst->len + 1, sizeof(char*)), .idx = 0 };
    lst_enum(lst, (list_item_h)on_make_cmd, (void*)&x, 0);
    lst_free(lst);
    return x.cmd;
}

void free_cmd (char **cmd) {
    char **p = cmd;
    while (*p) free(*p++);
    free(cmd);
}

void run (const char *cmd, size_t cmd_len, run_t *proc, int flags) {
    #ifndef __WIN32__
    int status;
    proc->cmd.ptr = (char*)cmd;
    proc->cmd.len = cmd_len;
    proc->pid = fork();
    if (-1 == proc->pid) return;
    if (0 == proc->pid) {
        char **args = mkcmdstr(cmd, cmd_len), **p = args;
        execvp(args[0], (char * const*)args);
        while (*p) free(*p++);
        free(args);
        kill(getpid(), SIGKILL);
    }
    if (RUN_WAIT == flags && proc->pid == waitpid(proc->pid, &status, 0)) {
        if (WIFEXITED(status)) { proc->run_exit = RUN_EXITED; proc->exit_code = WEXITSTATUS(status); } else
        if (WIFSTOPPED(status)) { proc->run_exit = RUN_STOPPED; proc->exit_code = WSTOPSIG(status); } else
        if (WIFSIGNALED(status)) { proc->run_exit = RUN_SIGNALED; proc->exit_code = WTERMSIG(status); };
        proc->pid = 0;
    }
    #else
    char cmdline [cmd_len+1];
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&proc->pi, sizeof(PROCESS_INFORMATION));
    strncpy(cmdline, cmd, cmd_len);
    cmdline[cmd_len] = '\0';
    if (!CreateProcess(NULL, cmdline, NULL, NULL, 0, 0, NULL, NULL, &si, &proc->pi)) return;
    proc->exit_code = WaitForSingleObject(proc->pi.hProcess, INFINITE);
    CloseHandle(proc->pi.hProcess);
    CloseHandle(proc->pi.hThread);
    #endif
}

int task_setopt_msg (task_t *task, task_opt_t opt, msg_h arg) {
    switch (opt) {
        case TASK_MSG: task->on_msg = arg; return 0;
        default: return -1;
    }
}

int task_setopt_msgfree (task_t *task, task_opt_t opt, msgfree_h arg) {
    switch (opt) {
        case TASK_FREEMSG: task->on_freemsg = arg; return 0;
        default: return -1;
    }
}

int task_setopt_int (task_t *task, task_opt_t opt, long arg) {
    switch (opt) {
        case TASK_MAXSLOTS:
            if (arg < 0)
                task->max_slots = sysconf(_SC_NPROCESSORS_ONLN);
            else
                task->max_slots = arg;
            return 0;
        case TASK_TIMEOUT:
            if (arg < 0)
                return -1;
            task->timeout = arg;
            return 0;
        default: return -1;
    }
}

task_t *task_create () {
    task_t *task = calloc(1, sizeof(task_t));
    pthread_mutex_init(&task->locker, NULL);
    pthread_cond_init(&task->cond, NULL);
    return task;
}

static msg_t *get_next_msg (slot_t *slot, task_t *task) {
    list_item_t *li;
    msg_t *msg = NULL;
    pthread_mutex_lock(&task->locker);
    while (slot->is_alive && !msg) {
        if ((li = task->queue->head)) {
            msg = (msg_t*)li->ptr;
            lst_del(li);
        } else
            pthread_cond_wait(&task->cond, &task->locker);
    }
    pthread_mutex_unlock(&task->locker);
    return msg;
}

static void *slot_process (void *param) {
    slot_t *slot = (slot_t*)param;
    task_t *task = slot->task;
    msg_t *msg = NULL;
    while ((msg = get_next_msg(slot, task))) {
        if (task->on_msg)
            task->on_msg(msg->req);
        if ((msg->flags & MSG_MUSTFREE) && task->on_freemsg)
            task->on_freemsg(msg->req);
        free(msg);
    }
    return NULL;
}

static void *task_process (void *param) {
    task_t *task = (task_t*)param;
    while (task->is_alive) {
        struct timespec ts;
        pthread_mutex_lock(&task->locker);
        clock_gettime(CLOCK_REALTIME, &ts);
        long ns = ts.tv_nsec+((task->timeout%1000)*1000000);
        ts.tv_sec += task->timeout/1000+(ns/1000000000);
        ts.tv_nsec = ns%1000000000;
        int rc = pthread_cond_timedwait(&task->cond, &task->locker, &ts);
        pthread_mutex_unlock(&task->locker);
        if (ETIMEDOUT == rc && task->on_msg)
            task->on_msg(NULL);
    }
    return NULL;
}

static void on_free_slot (slot_t *slot, void *dummy) {
    free(slot);
}

void task_start (task_t *task) {
    task->is_alive = 1;
    if (task->timeout > 0) {
        pthread_create(&task->th, NULL, task_process, task);
        return;
    }
    task->queue = lst_alloc(NULL);
    if (task->max_slots <= 0)
        task->max_slots = 0;
    task->slots = lst_alloc((free_h)on_free_slot);
    for (long i = 0; i < task->max_slots; ++i) {
        slot_t *slot = calloc(1, sizeof(slot_t));
        slot->is_alive = 1;
        slot->task = task;
        lst_adde(task->slots, slot);
        pthread_create(&slot->th, NULL, slot_process, slot);
    }
}

void task_cast (task_t *task, void *data, int flags) {
    if (!task->is_alive)
        return;
    msg_t *msg = calloc(1, sizeof(msg_t));
    msg->flags = flags;
    msg->req = data;
    pthread_mutex_lock(&task->locker);
    lst_adde(task->queue, msg);
    pthread_cond_broadcast(&task->cond);
    pthread_mutex_unlock(&task->locker);
}

static int on_send_stop (list_item_t *li, void *dummy) {
    slot_t *slot = (slot_t*)li->ptr;
    slot->is_alive = 0;
    return ENUM_CONTINUE;
}

static int on_wait_slot (list_item_t *li, void *dummy) {
    pthread_join(((slot_t*)li->ptr)->th, NULL);
    return ENUM_CONTINUE;
}

static int on_free_queue (list_item_t *li, task_t *task) {
    msg_t *msg = (msg_t*)li->ptr;
    if ((msg->flags & MSG_MUSTFREE) && task->on_freemsg)
        task->on_freemsg(msg);
    free(msg);
    return ENUM_CONTINUE;
}

void task_destroy (task_t *task) {
    task->is_alive = 0;
    if (!task->timeout) {
        pthread_mutex_lock(&task->locker);
        lst_enum(task->queue, (list_item_h)on_free_queue, task, 0);
        pthread_mutex_unlock(&task->locker);
        lst_enum(task->slots, on_send_stop, NULL, 0);
        pthread_mutex_lock(&task->locker);
        pthread_cond_broadcast(&task->cond);
        pthread_mutex_unlock(&task->locker);
        lst_enum(task->slots, on_wait_slot, NULL, 0);
        lst_free(task->slots);
        lst_enum(task->queue, (list_item_h)on_free_queue, task, 0);
        lst_free(task->queue);
    } else {
        pthread_mutex_lock(&task->locker);
        pthread_cond_signal(&task->cond);
        pthread_mutex_unlock(&task->locker);
        pthread_join(task->th, NULL);
    }
    pthread_cond_destroy(&task->cond);
    pthread_mutex_destroy(&task->locker);
    free(task);
}
