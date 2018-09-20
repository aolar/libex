#include "thread.h"

int mkthread (thread_t *th, thread_h proc, void *arg) {
    #ifndef __WIN32__
    return pthread_create(&th->h, NULL, proc, arg);
    #else
    return NULL != (th->h = CreateThread(0, 0, proc, arg, 0, &th->tid)) ? 0 : -1;
    #endif
}

int lock_init (lock_t *l) {
    #ifndef __WIN32__
    if (0 != pthread_mutexattr_init(&l->ma)) return -1;
    return pthread_mutex_init(&l->m, &l->ma);
    #else
    InitializeCriticalSection(&l->cs);
    return 0;
    #endif
}

int lock_done (lock_t *l) {
    #ifndef __WIN32__
    if (0 != pthread_mutex_destroy(&l->m)) return -1;
    return pthread_mutexattr_destroy(&l->ma);
    #else
    DeleteCriticalSection(&l->cs);
    return 0;
    #endif
}

int lock (lock_t *l) {
    #ifndef __WIN32__
    return pthread_mutex_lock(&l->m);
    #else
    EnterCriticalSection(&l->cs);
    return 0;
    #endif
}

int unlock (lock_t *l) {
    #ifndef __WIN32__
    return pthread_mutex_unlock(&l->m);
    #else
    LeaveCriticalSection(&l->cs);
    return 0;
    #endif
}
