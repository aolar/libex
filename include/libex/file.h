#ifndef __LIBEX_FILE_H__
#define __LIBEX_FILE_H__

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/file.h>
#include <dirent.h>
#include <fcntl.h>
#include "str.h"
#include "list.h"

//typedef void (*slogf_h) (const char*, ...);
//typedef void (*vslogf_h) (int, const char*, va_list);

//extern slogf_h slogf;
//extern vslogf_h vslogf;

int loginit (const char *fname);
void slogf (const char *fmt, ...);
void vslogf (int is_print_time, const char *fmt, va_list ap);

typedef void (*load_conf_h) (const char*, strptr_t*, strptr_t*);
int load_conf (const char *fname, load_conf_h fn);
int load_conf_exactly (const char *fname, load_conf_h fn);

#define CONF_HANDLER ({ void fn (const char *fname, strptr_t *key, strptr_t *val) {
#define CONF_HANDLER_END } fn; })
#define ASSIGN_CONF_STR(VAL,STR) \
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(STR))) { \
        if (VAL) strput(&VAL, val->ptr, val->len, 0); else VAL = mkstr(val->ptr, val->len, 8); \
        STR_ADD_NULL(VAL); \
        return; \
    }
#define ASSIGN_CONF_INT(VAL,STR) \
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(STR))) { \
        char *str = strndup(val->ptr, val->len), *tail; \
        long long temp##VAL = strtoll(str, &tail, 0); \
        if (*tail == '\0' && ERANGE != errno) VAL = temp##VAL; \
        free(str); \
    }
#define ASSIGN_CONF_DOUBLE(VAL,STR) \
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(STR))) { \
        char *str = strndup(val->ptr, val->len), *tail; \
        long double temp##VAL = strtold(str, &tail); \
        if (*tail == '\0' && ERANGE != errno) VAL = temp##VAL; \
        free(str); \
    }
#define ASSIGN_CONF_BOOL(VAL,STR) \
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(STR))) { \
        if (0 == cmpstr(val->ptr, val->len, CONST_STR_LEN("0"))) VAL = 0; else \
        if (0 == cmpstr(val->ptr, val->len, CONST_STR_LEN("1"))) VAL = 1; else \
        if (0 == cmpcasestr(val->ptr, val->len, CONST_STR_LEN("FALSE"))) VAL = 0; else \
        if (0 == cmpcasestr(val->ptr, val->len, CONST_STR_LEN("TRUE"))) VAL = 1; else \
        if (0 == cmpcasestr(val->ptr, val->len, CONST_STR_LEN("NO"))) VAL = 0; else \
        if (0 == cmpcasestr(val->ptr, val->len, CONST_STR_LEN("YES"))) VAL = 1; \
    }
#define S_INDIR 0x00008000

typedef int (*path_h) (const char*, void*, int, struct stat*);
void ls (const char *path, path_h on_path, void *userdata, int flags, int max_depath);

#ifdef __WIN32__
ssize_t getline (char **lineptr, size_t *n, FILE *fd);
#endif

int getfname (const char *path, size_t path_len, strptr_t *result);
int getoname (const char *path, size_t path_len, strptr_t *result);
void getdir (const char *path, size_t path_len, strptr_t *result);
int getsuf (const char *path, size_t path_len, strptr_t *result);

str_t *path_combine (const char *arg, ...);
str_t *path_add_path (str_t *path, const char *arg, ...);

str_t *mktempnam (const char *dir, size_t dir_len, const char *prefix, size_t prefix_len);

void path_split (const char *path, size_t path_len, str_t **dir, str_t **fname, str_t **suf);
str_t *path_expand (const char *where, size_t where_len, const char *path, size_t path_len);

int is_abspath (const char *path, size_t len);
#ifndef __WIN32__
#define MAX_PATH 1024
#endif

typedef enum {
    DIR_HOME,
    DIR_USR_CONFIG,
    DIR_LOCAL_CONFIG,
    DIR_HOME_CONFIG,
    DIR_CONFIG,
    DIR_PROGRAMS,
    DIR_CURRENT,
    DIR_TEMPATH
} spec_path_t;

str_t *get_spec_path (spec_path_t id);

str_t *load_all_file (const char *path, size_t chunk_size, size_t max_size);
int save_file (const char *path, const char *buf, size_t buf_len);
int is_file_exists (const char *path);
int copy_file (const char *src, const char *dst);

#endif // __LIBEX_FILE_H__
