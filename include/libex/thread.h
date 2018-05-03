#ifndef __LIBEX_THREAD_H__
#define __LIBEX_THREAD_H__

#include <stdlib.h>
#include <errno.h>
#include <stdint.h>
#include <stdarg.h>
#ifndef __WIN32__
#include <pthread.h>
#else
#include <windows.h>
#endif
#include <time.h>
#include "list.h"
#include "tree.h"

typedef struct {
    #ifndef __WIN32__
    pthread_t h;
    #else
    HANDLE h;
    DWORD tid;
    #endif
} thread_t;

#ifndef __WIN32__
typedef void* (*thread_h) (void*);
#else
typedef DWORD (WINAPI *thread_h) (void*);
#endif

int mkthread (thread_t *th, thread_h proc, void *arg);

typedef struct {
    #ifndef __WIN32__
    pthread_mutex_t m;
    pthread_mutexattr_t ma;
    #else
    CRITICAL_SECTION cs;
    #endif
} lock_t;

int lock_init (lock_t *l);
int lock_done (lock_t *l);
int lock (lock_t *l);
int unlock (lock_t *l);

#endif // __LIBEX_THREAD_H__
