#include "task.h"

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

int pool_setopt_int (pool_t *pool, pool_opt_t opt, long arg) {
    if (pool->is_alive)
        return -1;
    switch (opt) {
        case POOL_MAXSLOTS:
            if (arg < 0)
                pool->max_slots = sysconf(_SC_NPROCESSORS_ONLN);
            else
                pool->max_slots = arg;
            return 0;
        case POOL_TIMEOUT:
            if (arg < 0)
                return -1;
            pool->timeout = arg;
            return 0;
        case POOL_LIVINGTIME:
            if (arg < 0)
                return -1;
            pool->livingtime = arg;
            return 0;
        default: return -1;
    }
}

int pool_setopt_msg (pool_t *pool, pool_opt_t opt, pool_msg_h arg) {
    if (pool->is_alive)
        return -1;
    switch (opt) {
        case POOL_MSG:
            pool->on_msg = arg;
            return 0;
        default: return -1;
    }
}

int pool_setopt_create (pool_t *pool, pool_opt_t opt, pool_create_h arg) {
    if (pool->is_alive)
        return -1;
    switch (opt) {
        case POOL_CREATESLOT:
            pool->on_create_slot = arg;
            return 0;
        default: return -1;
    }
}

int pool_setopt_destroy (pool_t *pool, pool_opt_t opt, pool_destroy_h arg) {
    if (pool->is_alive)
        return -1;
    switch (opt) {
        case POOL_DESTROYSLOT:
            pool->on_destroy_slot = arg;
            return 0;
        case POOL_FREEMSG:
            pool->on_freemsg = arg;
            return 0;
        default: return -1;
    }
}

pool_t *pool_create () {
    pool_t *pool = calloc(1, sizeof(pool_t));
    return pool;
}

static msg_t *get_next_msg (slot_t *slot, pool_t *pool) {
    list_item_t *li;
    msg_t *msg = NULL;
    pthread_mutex_lock(&pool->locker);
    while (slot->is_alive && !msg) {
        if ((li = pool->queue->head)) {
            msg = (msg_t*)li->ptr;
            lst_del(li);
        } else
        if (0 == pool->livingtime)
            pthread_cond_wait(&pool->cond, &pool->locker);
        else {
            struct timespec totime;
            clock_gettime(CLOCK_MONOTONIC, &totime);
            totime.tv_sec += pool->livingtime;
            if (ETIMEDOUT == pthread_cond_timedwait(&pool->cond, &pool->locker, &totime))
                slot->is_alive = 0;
        }
    }
    pthread_mutex_unlock(&pool->locker);
    return msg;
}

typedef struct {
    slot_t *slot;
    void *init_data;
} slot_data_t;

static void *slot_process (void *arg) {
    msg_t *msg = NULL;
    slot_data_t *sd = (slot_data_t*)arg;
    slot_t *slot = sd->slot;
    pool_t *pool = slot->pool;
    pthread_mutex_lock(&pool->locker);
    slot->node =lst_adde(pool->slots, slot);
    pthread_mutex_unlock(&pool->locker);
    if (pool->on_create_slot) {
        if (-1 == pool->on_create_slot(slot, sd->init_data)) {
            pthread_mutex_lock(&pool->locker);
            lst_del(slot->node);
            if (pool->on_destroy_slot)
                pool->on_destroy_slot(slot);
            free(slot);
            pthread_mutex_unlock(&pool->locker);
        }
    }
    free(sd);
    while ((msg = get_next_msg(slot, pool))) {
        if (msg->on_msg) {
            int msg_rc = msg->on_msg(slot->data, msg->in_data, msg->out_data);
            if (msg->mutex && msg->cond) {
                pthread_mutex_lock(msg->mutex);
                pthread_cond_signal(msg->cond);
                pthread_mutex_unlock(msg->mutex);
            }
            if (MSG_DONE == msg_rc) {
                if (pool->on_freemsg)
                    pool->on_freemsg(msg->in_data);
                free(msg);
            } else
            if (msg_rc == MSG_CONTINUE) {
                if (-1 == pool_call(pool, msg, slot->data)) {
                    if (pool->on_freemsg)
                        pool->on_freemsg(msg->in_data);
                }
            }
        }
    }
    pthread_mutex_lock(&pool->locker);
    if (pool->on_destroy_slot)
        pool->on_destroy_slot(slot);
    lst_del(slot->node);
    pthread_mutex_unlock(&pool->locker);
    free(slot);
    return NULL;
}

static void *pool_process (void *param) {
    pool_t *pool = (pool_t*)param;
    while (pool->is_alive) {
        struct timespec ts;
        pthread_mutex_lock(&pool->locker);
        clock_gettime(CLOCK_MONOTONIC, &ts);
        long ns = ts.tv_nsec+((pool->timeout%1000)*1000000);
        ts.tv_sec += pool->timeout/1000+(ns/1000000000);
        ts.tv_nsec = ns%1000000000;
        int rc = pthread_cond_timedwait(&pool->cond, &pool->locker, &ts);
        pthread_mutex_unlock(&pool->locker);
        if (ETIMEDOUT == rc && pool->on_msg)
            pool->on_msg(NULL, NULL, NULL);
    }
    return NULL;
}

static int add_slot (pool_t *pool, void *init_data) {
    slot_data_t *sd = malloc(sizeof(slot_data_t));
    slot_t *slot = sd->slot = calloc(1, sizeof(slot_t));
    slot->is_alive = 1;
    slot->pool = pool;
    sd->init_data = init_data;
    if (0 != pthread_create(&slot->th, NULL, slot_process, sd)) {
        free(slot);
        free(sd);
        return -1;
    }
    pthread_detach(slot->th);
    return 0;
}

void pool_start (pool_t *pool) {
    pool->is_alive = 1;
    pthread_mutex_init(&pool->locker, NULL);
    if (pool->livingtime || pool->timeout) {
        pthread_condattr_init(&pool->cond_attr);
        pthread_condattr_setclock(&pool->cond_attr, CLOCK_MONOTONIC);
        pthread_cond_init(&pool->cond, &pool->cond_attr);
    } else
        pthread_cond_init(&pool->cond, NULL);
    if (pool->timeout > 0) {
        pthread_create(&pool->th, NULL, pool_process, pool);
        return;
    }
    pool->queue = lst_alloc(NULL);
    if (pool->max_slots <= 0)
        pool->max_slots = 0;
    pool->slots = lst_alloc(NULL);
    if (0 == pool->livingtime)
        for (long i = 0; i < pool->max_slots; ++i)
            add_slot(pool, NULL);
}

msg_t *pool_createmsg (pool_msg_h on_msg, void *in_data, void *out_data, pthread_mutex_t *mutex, pthread_cond_t *cond) {
    msg_t *msg = calloc(1, sizeof(msg_t));
    msg->in_data = in_data;
    msg->out_data = out_data;
    msg->on_msg = on_msg;
    msg->mutex = mutex;
    msg->cond = cond;
    return msg;
}

int pool_call (pool_t *pool, msg_t *msg, void *init_data) {
    int ret = 0;
    if (!pool->is_alive)
        return -1;
    pthread_mutex_lock(&pool->locker);
    if (pool->livingtime > 0 && (0 == pool->slots->len || (pool->queue->len > 0 && pool->slots->len < pool->max_slots)))
        ret = add_slot(pool, init_data);
    if (0 != ret)
        return ret;
    lst_adde(pool->queue, msg);
    pthread_cond_broadcast(&pool->cond);
    pthread_mutex_unlock(&pool->locker);
    return ret;
}

int pool_callmsgwait (pool_t *pool, pool_msg_h on_msg, void *in_data, void *out_data, void *init_data) {
    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    pthread_mutex_lock(&mutex);
    msg_t *msg = pool_createmsg(on_msg, in_data, out_data, &mutex, &cond);
    int rc = pool_call(pool, msg, init_data);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);
    pthread_cond_destroy(&cond);
    pthread_mutex_destroy(&mutex);
    return rc;
}

typedef struct {
    pthread_t *ths;
    int idx;
} slot_stop_t;

static int on_send_stop (list_item_t *li, void *data) {
    slot_t *slot = (slot_t*)li->ptr;
    slot->is_alive = 0;
    return ENUM_CONTINUE;
}

static int on_free_queue (list_item_t *li, pool_t *pool) {
    msg_t *msg = (msg_t*)li->ptr;
    free(msg);
    return ENUM_CONTINUE;
}

void pool_destroy (pool_t *pool) {
    pool->is_alive = 0;
    if (!pool->timeout) {
        pthread_mutex_lock(&pool->locker);
        slot_stop_t ss = { .ths = NULL, .idx = 0 };
        lst_enum(pool->slots, on_send_stop, (void*)&ss, 0);
        pthread_cond_broadcast(&pool->cond);
        pthread_mutex_unlock(&pool->locker);
        sleep(1);
        lst_free(pool->slots);
        lst_enum(pool->queue, (list_item_h)on_free_queue, pool, 0);
        lst_free(pool->queue);
    } else {
        pthread_mutex_lock(&pool->locker);
        pthread_cond_signal(&pool->cond);
        pthread_mutex_unlock(&pool->locker);
        pthread_join(pool->th, NULL);
    }
    if (pool->livingtime || pool->timeout)
        pthread_condattr_destroy(&pool->cond_attr);
    pthread_cond_destroy(&pool->cond);
    pthread_mutex_destroy(&pool->locker);
    free(pool);
}
