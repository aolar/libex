/**
 * @file file.h
 * @brief file functions
 */
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

/**
 * @brief initialize log file
 * @param fname log file filename
 * @retval 0 if success, -1 if file can't created
 */
int loginit (const char *fname);

/**
 * @brief write string to log file
 * @param fmt same as parameters \b snprintf
 */
void slogf (const char *fmt, ...);
/**
 * @brief write string to log file
 * @param is_print_time
 * @param fmt same as parameters \b vsnprintf
 * @param ap
 */
void vslogf (int is_print_time, const char *fmt, va_list ap);

/**
 * @brief callback for load configuration file
 */
typedef void (*load_conf_h) (const char*, strptr_t*, strptr_t*);

/**
 * @brief load configuration file
 * @param fname configuration filename
 * @param fn callback function
 * @retval 0 if success
 */
int load_conf (const char *fname, load_conf_h fn);

/**
 * @brief load exactly configuration file
 * @param fname configuration filename
 * @param fn callback function
 * @retval 0 if success
 */
int load_conf_exactly (const char *fname, load_conf_h fn);

/**
 * @brief start handler
 */
#define CONF_HANDLER ({ void fn (const char *fname, strptr_t *key, strptr_t *val) {

/**
 * @brief end handler
 */
#define CONF_HANDLER_END } fn; })

/**
 * @brief assign string configuration item
 */
#define ASSIGN_CONF_STR(VAL,STR) \
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(STR))) { \
        if (VAL) strput(&VAL, val->ptr, val->len, 0); else VAL = mkstr(val->ptr, val->len, 8); \
        STR_ADD_NULL(VAL); \
        return; \
    }

/**
 * @brief assign integer configuration item
 */
#define ASSIGN_CONF_INT(VAL,STR) \
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(STR))) { \
        char *str = strndup(val->ptr, val->len), *tail; \
        long long temp##VAL = strtoll(str, &tail, 0); \
        if (*tail == '\0' && ERANGE != errno) VAL = temp##VAL; \
        free(str); \
    }

/**
 * @brief assign double configuration item
 */
#define ASSIGN_CONF_DOUBLE(VAL,STR) \
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(STR))) { \
        char *str = strndup(val->ptr, val->len), *tail; \
        long double temp##VAL = strtold(str, &tail); \
        if (*tail == '\0' && ERANGE != errno) VAL = temp##VAL; \
        free(str); \
    }

/**
 * @brief assign boolean configuration item
 */
#define ASSIGN_CONF_BOOL(VAL,STR) \
    if (0 == cmpstr(key->ptr, key->len, CONST_STR_LEN(STR))) { \
        if (0 == cmpstr(val->ptr, val->len, CONST_STR_LEN("0"))) VAL = 0; else \
        if (0 == cmpstr(val->ptr, val->len, CONST_STR_LEN("1"))) VAL = 1; else \
        if (0 == cmpcasestr(val->ptr, val->len, CONST_STR_LEN("FALSE"))) VAL = 0; else \
        if (0 == cmpcasestr(val->ptr, val->len, CONST_STR_LEN("TRUE"))) VAL = 1; else \
        if (0 == cmpcasestr(val->ptr, val->len, CONST_STR_LEN("NO"))) VAL = 0; else \
        if (0 == cmpcasestr(val->ptr, val->len, CONST_STR_LEN("YES"))) VAL = 1; \
    }

/**
 * start directory processing
 */
#define S_INDIR 0x00008000

/**
 * @brief A callback function used \b ls function
 */
typedef int (*path_h) (const char*, void*, int, struct stat*);

/**
 * @brief list files and directories
 * @param path
 * @param on_path
 * @param userdata
 * @param flags
 * <ul>
 *  <li>\b ENUM_STOP_IF_BREAK if callback function returns \b ENUM_BREAK then listing directory is stopped
 * </ul>
 * @param max_depath
 */
void ls (const char *path, path_h on_path, void *userdata, int flags, int max_depath);

#ifdef __WIN32__
ssize_t getline (char **lineptr, size_t *n, FILE *fd);
#endif

/**
 * @brief extract file name from full path
 * @param path source path
 * @param path_len source path length
 * @param result file name
 * @retval 0 if success, -1 if string not connains file name
 */
int getfname (const char *path, size_t path_len, strptr_t *result);

/**
 * @brief extract file name without suffix from full path
 * @param path source path
 * @param path_len source path length
 * @param result file name
 * @retval 0 if success, -1 if string not connains file name
 */
int getoname (const char *path, size_t path_len, strptr_t *result);

/**
 * @brief extract directory from full path
 * @param path source path
 * @param path_len source path length
 * @param result directory
 * @retval 0 if success, -1 if string not connains file name
 */
void getdir (const char *path, size_t path_len, strptr_t *result);

/**
 * @brief extract suffix from full path
 * @param path source path
 * @param path_len source path length
 * @param result suffix
 * @retval 0 if success, -1 if string not connains file name
 */
int getsuf (const char *path, size_t path_len, strptr_t *result);

/**
 * @brief create path from strings
 * @param arg argument
 * @retval full path
 */
str_t *path_combine (const char *arg, ...);

/**
 * @brief add strings to full path
 * @param path source path
 * @param arg argumens
 * @retval new full path
 */
str_t *path_add_path (str_t *path, const char *arg, ...);

/**
 * @brief create random file name
 * @param dir directory for file name
 * @param dir_len string length of \b dir
 * @param prefix prefix for file name
 * @param prefix_len string length for \b prefix_len
 * @retval full path if success, NULL if error
 */
str_t *mktempnam (const char *dir, size_t dir_len, const char *prefix, size_t prefix_len);

/**
 * @brief split full path to directory, file name and suffix
 * @param path source full path
 * @param path_len string length of \b path
 * @param dir result directory
 * @param fname result file name
 * @param suf result suffix
 */
void path_split (const char *path, size_t path_len, str_t **dir, str_t **fname, str_t **suf);

/**
 * @brief expands relative path to full path
 * @param where
 * @param where_len
 * @param path
 * @param path_len
 * @retval full path if success, NULL if error
 */
str_t *path_expand (const char *where, size_t where_len, const char *path, size_t path_len);

/**
 * @brief function checks whether path is absolute
 * @param path
 * @param len
 * @retval returns 1 if path is absolute, 0 if not
 */
int is_abspath (const char *path, size_t len);

/**
 * maximum path length
 */
#ifndef __WIN32__
#define MAX_PATH 1024
#endif

/**
 * @brief kind of directory for \b get_spec_path function
 */
typedef enum {
    /**
     * home directory
     */
    DIR_HOME,
    /**
     * usr configuration directory
     */
    DIR_USR_CONFIG,
    /**
     * local configuration directory
     */
    DIR_LOCAL_CONFIG,
    /**
     * current user configuration directory
     */
    DIR_HOME_CONFIG,
    /**
     * global configuration directory
     */
    DIR_CONFIG,
    /**
     * program files directory
     */
    DIR_PROGRAMS,
    /**
     * current directory
     */
    DIR_CURRENT,
    /**
     * temporary files directory
     */
    DIR_TEMPATH
} spec_path_t;

/**
 * @brief returns special path name
 * @param id special directory identifier
 * @retval path
 */
str_t *get_spec_path (spec_path_t id);

/**
 * @brief load all file into buffer
 * @param path
 * @param chunk_size chunk size result string
 * @param max_size maximum file size, if \b max_size == 0 then maximum filname is unlimited
 * @retval path, NULL if error or file size greater then \b max_size, check errno
 */
str_t *load_all_file (const char *path, size_t chunk_size, size_t max_size);

/**
 * @brief save buffer to file
 * @param path
 * @param buf
 * @param buf_len buffer length
 * @retval 0 if success, -1 if error, check errno
 */
int save_file (const char *path, const char *buf, size_t buf_len);

/**
 * @brief check whether file is exists
 * @param path
 * @retval 0 if file is exists, else returns errno
 */
int is_file_exists (const char *path);

/**
 * @brief copy file to other location
 * @param src source file name
 * @param dst destination file name
 * @retval 0 if success, -1 if error, check errno
 */
int copy_file (const char *src, const char *dst);

#endif // __LIBEX_FILE_H__
