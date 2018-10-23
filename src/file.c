#include "file.h"

const char *log_fname = NULL;
#ifndef __WIN32__
#define PATH_DELIM_C '/'
#define PATH_DELIM_S "/"
#else
#define PATH_DELIM_C '\\'
#define PATH_DELIM_S "\\"
#endif

static void std_slogf (const char *fmt, ...) {}
static void std_vslogf (int is_print_time, const char *fmt, va_list va) {}

static void fl_vslogf (int is_print_time, const char *fmt, va_list ap);
static void fl_slogf (const char *fmt, ...);

slogf_h slogf = std_slogf;
vslogf_h vslogf = std_vslogf;

int loginit (const char *fname) {
    struct stat st;
    if (-1 == stat(fname, &st)) {
        if (errno == ENOENT) {
            #ifndef __WIN32__
            int fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, 0644);
            #else
            int fd = open(fname, O_WRONLY | O_CREAT | O_APPEND);
            #endif
            if (fd >= 0) close(fd); else return -1;
            slogf = fl_slogf;
            vslogf = fl_vslogf;
        } else
            return -1;
    }
    log_fname = fname;
    return 0;
}

#define LOG_BUF_SIZE 1024

static void fl_vslogf (int is_print_time, const char *fmt, va_list ap) {
    if (log_fname) {
        char buf [LOG_BUF_SIZE] = {'\0'};
        time_t t = time(0);
        const struct tm *tm;
        int len = 0, fd;
        if (is_print_time) {
            tm = localtime(&t);
            len += strftime(buf, LOG_BUF_SIZE-2, "%a, %d %b %Y %H:%M:%S GMT ", tm);
            //len = strlen(buf);
        }
        len += vsnprintf(buf+len, LOG_BUF_SIZE-len-2, fmt, ap);
        buf[len++] = '\n';
        #ifndef __WIN32__
        if (-1 != (fd = open(log_fname, O_WRONLY | O_CREAT | O_APPEND, 0644))) {
        #else
        if (-1 != (fd = open(log_fname, O_WRONLY | O_CREAT | O_APPEND))) {
        #endif
            flock(fd, LOCK_EX);
            write(fd, buf, len);
            flock(fd, LOCK_UN);
            close(fd);
        }
    }
}

static void fl_slogf (const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vslogf(1, fmt, ap);
    va_end(ap);
}

int load_conf_exactly (const char *fname, load_conf_h fn) {
    struct stat st;
    int fd;
    if (-1 == stat(fname, &st)) return -1;
    if (-1 == (fd = open(fname, O_RDONLY))) return -1;
    char *buf = malloc(st.st_size);
    ssize_t readed = read(fd, buf, st.st_size);
    close(fd);
    if (st.st_size != readed) {
        free(buf);
        return -1;
    }
    strptr_t str = { readed, buf };
    strptr_t line;
    while (-1 != strntok(&str.ptr, &str.len, CONST_STR_LEN("\r\n"), &line)) {
        char *p = strnchr(line.ptr, '#', line.len);
        if (p)
            line.len -= (uintptr_t)(line.ptr + line.len) - (uintptr_t)p;
        strptr_t name, val;
        if (0 == strntok(&line.ptr, &line.len, CONST_STR_LEN("="), &name) && -1 != strntok(&line.ptr, &line.len, CONST_STR_LEN("\r\n"), &val))
            fn(fname, &name, &val);
    }
    free(buf);
    return 0;
}

static int load_conf_from_spec (spec_path_t id, const char *fname, load_conf_h fn, int hidden) {
    str_t *path = get_spec_path(id);
    if (!path) return -1;
    if (hidden) {
        path = path_add_path(path, ".", NULL);
        strnadd(&path, fname, strlen(fname));
        STR_ADD_NULL(path);
    } else
        path = path_add_path(path, fname, NULL);
    int rc = load_conf_exactly(path->ptr, fn);
    free(path);
    return rc;
}

int load_conf (const char *fname, load_conf_h fn) {
    int rc = -1;
    if (0 == load_conf_from_spec(DIR_CONFIG, fname, fn, 0)) rc = 0;
    if (0 == load_conf_from_spec(DIR_USR_CONFIG, fname, fn, 0)) rc = 0;
    if (0 == load_conf_from_spec(DIR_LOCAL_CONFIG, fname, fn, 0)) rc = 0;
    if (0 == load_conf_from_spec(DIR_HOME_CONFIG, fname, fn, 0)) rc = 0;
    if (0 == load_conf_from_spec(DIR_HOME, fname, fn, 1)) rc = 0;
    if (0 == load_conf_from_spec(DIR_CURRENT, fname, fn, 1)) rc = 0;
    return rc;
}

static int enumdir (const char *path, path_h on_path, void *userdata, int flags, int max_depth, int depth) {
    struct stat st;
    DIR *dp;
    if (max_depth > 0 && max_depth <= depth)
        return ENUM_BREAK;
    if (-1 == stat(path, &st))
        return ENUM_BREAK;
    if ((dp = opendir(path))) {
        struct dirent *ep;
        if (ENUM_BREAK == on_path(path, userdata, flags | S_INDIR, &st) && (flags & ENUM_STOP_IF_BREAK))
            return ENUM_BREAK;
        while ((ep = readdir(dp))) {
            str_t *cpath = path_combine(path, ep->d_name, NULL);
            struct stat st;
            if (0 != strcmp(ep->d_name, ".") && 0 != strcmp(ep->d_name, "..") && 0 == stat(cpath->ptr, &st)) {
                if (S_ISDIR(st.st_mode)) {
                    if (ENUM_BREAK == enumdir(cpath->ptr, on_path, userdata, flags, max_depth, depth+1) && (flags & ENUM_STOP_IF_BREAK)) {
                        closedir(dp);
                        return ENUM_BREAK;
                    }
                } else
                    if (ENUM_BREAK == on_path(cpath->ptr, userdata, flags, &st) && (flags & ENUM_STOP_IF_BREAK)) {
                        closedir(dp);
                        return ENUM_BREAK;
                    }
            }
            free(cpath);
        }
        closedir(dp);
        if (ENUM_BREAK == on_path(path, userdata, flags, &st) && (flags & ENUM_STOP_IF_BREAK))
            return ENUM_BREAK;
    }
    return ENUM_CONTINUE;
}

void ls (const char *path, path_h on_path, void *userdata, int flags, int max_depath) {
    enumdir(path, on_path, userdata, flags, max_depath, 0);
}

#ifdef __WIN32__
#define GL_BUF_INC 64
ssize_t getline (char **lineptr, size_t *n, FILE *fd) {
    int c;
    ssize_t i = 0;
    if (*n == 0 || *lineptr == NULL) {
        *n = GL_BUF_INC;
        *lineptr = realloc(*lineptr, *n);
    }
    while (EOF != (c = fgetc(fd))) {
        if (*n == i + 1) {
            *n += GL_BUF_INC;
            *lineptr = realloc(*lineptr, *n);
        }
        *((char*)(*lineptr + i)) = c;
        ++i;
        if (c == '\r') {
            c = fgetc(fd);
            if (*n == i + 1) {
                *n += GL_BUF_INC;
                *lineptr = realloc(*lineptr, *n);
            }
            *((char*)(*lineptr + i)) = c;
        }
        if (c == '\n')
            break;
    }
    if (c == EOF && i == 0)
        return -1;
    if (*n == i + 1) {
        *n += GL_BUF_INC;
        *lineptr = realloc(*lineptr, *n);
    }
    *((char*)(*lineptr + i)) = '\0';
    return i;
}
#endif

int getfname (const char *path, size_t path_len, strptr_t *result) {
    memset(result, 0, sizeof(strptr_t));
    if (0 == path_len) return -1;
    char *p = strnrchr(path, PATH_DELIM_C, path_len);
    if (!p) p = (char*)path;
    if (*p == PATH_DELIM_C) {
        if (1 == path_len) return -1;
        ++p;
    }
    result->ptr = p;
    result->len = path_len - ((uintptr_t)p - (uintptr_t)path);
    return 0;
}

int getoname (const char *path, size_t path_len, strptr_t *result) {
    if (-1 == getfname(path, path_len, result))
        return -1;
    char *p = strnrchr(result->ptr, '.', result->len);
    if (p)
        result->len = (uintptr_t)p - (uintptr_t)result->ptr;
    return 0;
}

str_t *path_add_path (str_t *path, const char *arg, ...) {
    va_list ap;
    va_start(ap, arg);
    if (arg) {
        const char *p = arg, *q;
        if (path->len == 0 || PATH_DELIM_C != path->ptr[path->len-1])
            strnadd(&path, CONST_STR_LEN(PATH_DELIM_S));
        q = arg;
        while (*q && *q == PATH_DELIM_C) ++q;
        if ('\0' != *q)
            strnadd(&path, q, strlen(q));
        while ((p = va_arg(ap, const char*))) {
            if (path->len == 0 || PATH_DELIM_C != path->ptr[path->len-1])
                strnadd(&path, CONST_STR_LEN(PATH_DELIM_S));
            while (*q && *q == PATH_DELIM_C) ++q;
            if ('\0' != *q)
                strnadd(&path, q, strlen(q));
        }
    }
    va_end(ap);
    STR_ADD_NULL(path);
    return path;
}

#define TEMP_NAMLEN 16
#define TEMP_TIMLEN 24
char templ ['9'-'0'+1+'Z'-'A'+1+'z'-'a'+1];
void init_templ () __attribute__ ((constructor));
void init_templ () {
    char *p = templ;
    char c = '0';
    while (c <= '9') { *p++ = c++; };
    c = 'A';
    while (c <= 'Z') { *p++ = c++; };
    c = 'a';
    while (c <= 'z') { *p++ = c++; };
    srand(time(0));
}
str_t *mktempnam (const char *dir, size_t dir_len, const char *prefix, size_t prefix_len) {
    char tempdir [MAX_PATH+1];
    size_t tempdir_len;
    strptr_t oname = { .ptr = NULL, .len = 0 };
    if (-1 == getoname(prefix, prefix_len, &oname))
        return NULL;
    if (!dir || !dir_len) {
        #ifdef __WIN32__
            tempdir_len = GetTempPath(tempdir, sizeof(tempdir));
        #else
            strcpy(tempdir, "/tmp");
            tempdir_len = sizeof("/tmp")-1;
        #endif
    } else {
        if (dir_len > sizeof(tempdir))
            return NULL;
        tempdir_len = dir_len;
        strncpy(tempdir, dir, dir_len);
    }
    str_t *path = stralloc(tempdir_len + prefix_len + TEMP_NAMLEN + TEMP_TIMLEN + 1, 16);
    strnadd(&path, tempdir, tempdir_len);
    if (oname.ptr && oname.len ) {
        if (path->ptr[path->len-1] != PATH_DELIM_C)
            strnadd(&path, CONST_STR_LEN("/"));
        strnadd(&path, oname.ptr, oname.len);
    }
    for (int i = 0; i < TEMP_NAMLEN; ++i)
        path->ptr[i+path->len] = templ[rand()%sizeof(templ)];
    char buf [TEMP_TIMLEN];
    snprintf(buf, TEMP_TIMLEN, TIME_FMT, time(0));
    size_t buflen = strlen(buf);
    //strncpy(path->ptr+path->len+TEMP_NAMLEN, buf, buflen);
    strcpy(path->ptr+path->len+TEMP_NAMLEN, buf);
    path->len += TEMP_NAMLEN+buflen;
    if (getsuf(prefix, prefix_len, &oname))
        strnadd(&path, oname.ptr, oname.len);
    STR_ADD_NULL(path);
    return path;
}

str_t *path_combine (const char *arg, ...) {
    str_t *path = mkstr(arg, strlen(arg), 16);
    va_list ap;
    va_start(ap, arg);
    const char *p;
    while ((p = va_arg(ap, const char*))) {
        if (path->len == 0 || PATH_DELIM_C != path->ptr[path->len-1])
            strnadd(&path, CONST_STR_LEN(PATH_DELIM_S));
        const char *q = p;
        while (*q && *q == PATH_DELIM_C) ++q;
        if ('\0' != *q)
            strnadd(&path, q, strlen(q));
    }
    va_end(ap);
    STR_ADD_NULL(path);
    return path;
}

void path_split (const char *path, size_t path_len, str_t **dir, str_t **fname, str_t **suf) {
    char *d = (char*)path, *f, *s;
    size_t dl = path_len, fl, sl;
    if ((f = strnrchr(path, PATH_DELIM_C, path_len))) {
        --dl;
        fl = dl - ((uintptr_t)f - (uintptr_t)path);
        dl -= fl;
        ++f;
        if ((s = strnchr(f, '.', fl))) {
            --fl;
            sl = fl - ((uintptr_t)s - (uintptr_t)f);
            fl -= sl;
            ++s;
        }
    } else {
        f = d;
        fl = dl;
        d = NULL;
        if ((s = strnrchr(f, '.', fl))) {
            sl = fl - ((uintptr_t)s - (uintptr_t)f);
            fl -= sl;
            ++s;
            --sl;
        }
    }
    *dir = *fname = *suf = NULL;
    if (d) { *dir = mkstr(d, dl, 8); STR_ADD_NULL(*dir); }
    if (f) { *fname = mkstr(f, fl, 8); STR_ADD_NULL(*fname); }
    if (s) { *suf = mkstr(s, sl, 8); STR_ADD_NULL(*suf); }
}

str_t *get_spec_path (spec_path_t id) {
    str_t *ret = NULL;
    const char *dirname = NULL;
    char buf [MAX_PATH+1];
    switch (id) {
        case DIR_HOME_CONFIG:
        #ifdef __WIN32__
            if ((dirname = SHGetSpecialFolderPath(0, buf, CSIDL_LOCAL_APPDATA, 1) ? buf : NULL))
                ret = mkstr(dirname, strlen(dirname), 16);
        #else
            if ((dirname = getenv("HOME")))
                ret = path_combine(dirname, ".config", NULL);
        #endif
            break;
        case DIR_HOME:
        #ifdef __WIN32__
            if ((dirname = SHGetSpecialFolderName(0, buf, CSIDL_PROFILE, 1) ? buf : NULL))
                ret = mkstr(dirname, strlen(dirname), 16);
        #else
            if ((dirname = getenv("HOME")))
                ret = mkstr(dirname, strlen(dirname), 16);
        #endif
            break;
        case DIR_CONFIG:
        #ifdef __WIN32__
            if ((dirname = SHGetSpecialFolderPath(0, buf, CSIDL_APPDATA, 1) ? buf : NULL))
                ret = mkstr(dirname, strlen(dirname), 16);
        #else
            ret = mkstr(CONST_STR_LEN("/etc"), 16);
        #endif
            break;
        case DIR_USR_CONFIG:
        #ifdef __WIN32__
            if ((dirname = SHGetSpecialFolderPath(0, buf, CSIDL_APPDATA, 1) ? buf : NULL))
                ret = mkstr(dirname, strlen(dirname), 16);
        #else
            ret = mkstr(CONST_STR_LEN("/usr/etc"), 16);
        #endif
            break;
        case DIR_LOCAL_CONFIG:
        #ifdef __WIN32__
            if ((dirname = SHGetSpecialFolderPath(0, buf, CSIDL_APPDATA, 1) ? buf : NULL))
                ret = mkstr(dirname, strlen(dirname), 16);
        #else
            ret = mkstr(CONST_STR_LEN("/usr/local/etc"), 16);
        #endif
            break;
        case DIR_PROGRAMS:
        #ifdef __WIN32__
            if ((dirname = SHGetSpecialFolderPath(0, buf, CSIDL_PROGRAM_FILES, 1) ? buf : NULL))
                ret = mkstr(dirname, strlen(dirname), 16);
        #else
            ret = mkstr(CONST_STR_LEN("/usr/bin"), 16);
        #endif
            break;
        case DIR_CURRENT:
            if (!getcwd(buf, sizeof buf))
                return NULL;
            ret = mkstr(buf, strlen(buf), 16);
            break;
        case DIR_TEMPATH:
        #ifdef __WIN32__
            GetTempPath(sizeof(buf), buf);
            ret = mkstr(buf, strlen(buf), 16);
        #else
            ret = mkstr(CONST_STR_LEN("/tmp"), 16);
        #endif
        break;
    }
    STR_ADD_NULL(ret);
    return ret;
}

str_t *path_expand (const char *where, size_t where_len, const char *path, size_t path_len) {
    const char *p;
    size_t len;
    int fin = -1;
    
    int patch_path () {
        len = (uintptr_t)p - (uintptr_t)path;
        if (1 == len && '.' == *path) {
            path = p+1;
            path_len -= len+1;
            return 1;
        }
        if (2 == len && '.' == *(path+1)) {
            path = p+1;
            path_len -= len+1;
            if (!(p = strnrchr(where, '/', where_len)))
                return -1;
            where_len = (uintptr_t)p - (uintptr_t)where;
            return 1;
        }
        return 0;
    }
    
    if ('/' == *path)
        return mkstr(path, path_len, 16);
    if ((p = (const char*)strnchr(path, '/', path_len))) {
        if (0 < (fin = patch_path()))
        while ((p = (const char*)strnchr(path, '/', path_len)))
            if (0 >= (fin = patch_path()))
                break;
    } else {
        p = path;
        fin = 0;
    }
    str_t *str = NULL;
    if (-1 == fin)
        return NULL;
    if (!(str = mkstr(where, where_len, 16)))
        return NULL;
    if (-1 == strnadd(&str, CONST_STR_LEN("/")) || -1 == strnadd(&str, path, path_len)) {
        free(str);
        return NULL;
    }
    STR_ADD_NULL(str);
    return str;
}

void getdir (const char *path, size_t path_len, strptr_t *result) {
    char *p = strnrchr(path, PATH_DELIM_C, path_len);
    result->ptr = (char*)path;
    result->len = 0;
    if (p)
        result->len = (uintptr_t)p - (uintptr_t)path;
}

int getsuf (const char *path, size_t path_len, strptr_t *result) {
    const char *p = path + path_len, *e = p;
    result->ptr = NULL;
    result->len = 0;
    while (p > path && *p != '.') --p;
    if (*p == '.') {
        result->ptr = (char*)p;
        result->len = (uintptr_t)e - (uintptr_t)p;
        return 1;
    }
    return 0;
}

int is_abspath (const char *path, size_t len) {
#ifdef __WIN32__
    return len >= 3 && isalpha(path[0]) && path[1] == ':' && path[2] == '\\' ? 1 : 0;
#else
    return len >= 1 && path[0] == '/';
#endif
}

str_t *load_all_file (const char *path, size_t chunk_size, size_t max_size) {
    str_t *ret = NULL;
    errno = 0;
    int fd = open(path, O_RDONLY);
    if (-1 != fd) {
        struct stat st;
        if (-1 != fstat(fd, &st) && ((max_size > 0 && st.st_size <= max_size) || 0 == max_size)) {
            ret = stralloc(st.st_size, chunk_size);
            if (st.st_size == read(fd, ret->ptr, st.st_size))
                ret->len = st.st_size;
            else {
                free(ret);
                ret = NULL;
            }
        }
        close(fd);
    }
    return ret;
}

int save_file (const char *path, const char *buf, size_t buf_len) {
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH), rc = -1;
    if (-1 != fd) {
        if (buf_len == write(fd, buf, buf_len))
            rc = 0;
        close(fd);
    }
    return rc;
}

int is_file_exists (const char *path) {
    struct stat st;
    int rc = stat(path, &st);
    if (-1 == rc) rc = errno;
    return rc;
}

int copy_file (const char *src, const char *dst) {
    int fd_src = open(src, O_RDONLY), rc = -1;
    if (fd_src > 0) {
        struct stat st;
        if (-1 != fstat(fd_src, &st)) {
            int fd_dst = open(dst, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
            if (fd_dst > 0) {
                ssize_t readed;
                char buf [4096];
                while ((readed = read(fd_src, buf, sizeof(buf))) > 0)
                    if (-1 == (rc = write(fd_dst, buf, readed)))
                        break;
                close(fd_dst);
                rc = rc < 0 ? -1 : 0;
            }
        }
        close(fd_src);
    }
    return rc;
}
