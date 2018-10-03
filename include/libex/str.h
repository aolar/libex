#ifndef __LIBEX_STR_H__
#define __LIBEX_STR_H__

#ifdef __WIN32__
#ifdef __STRICT_ANSI__
#undef __STRICT_ANSI__
#endif
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <locale.h>
#include <wchar.h>
#include <wctype.h>

#if __x86_64 || __ppc64__
#define LONG_FMT "%ld"
#define ULONG_FMT "%lu"
#define TIME_FMT "%lu"
#define SIZE_FMT "%lu"
#define LONG_HEXFMT "%02lX"
#else
#define LONG_FMT "%lld"
#define ULONG_FMT "%llu"
#define TIME_FMT "%lu"
#define SIZE_FMT "%u"
#define LONG_HEXFMT "%02llX"
#endif

#define STR_SPACES " \f\n\r\t\v"
#ifndef CONST_STR_LEN
#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
#define WCONST_STR_LEN(x) x, x ? (sizeof(x) / sizeof(wchar_t) - 1) : 0
#endif
#define CONST_STR_NULL NULL,0
#define STR_REDUCE 0x0001
#define STR_ADD_NULL(x) (x)->ptr[(x)->len] = '\0'
#define WSTR_ADD_NULL(x) (x)->ptr[(x)->len] = L'\0'
#define CONST_STR_INIT(s) { .len = sizeof(s)-1, .ptr = s }
#define CONST_STR_INIT_NULL { .len = 0, .ptr = NULL }

#define STR_LEFT 0x0001
#define STR_CENTER 0x0002
#define STR_RIGHT 0x0004
#define STR_MAYBE_UTF 0x8000

#define RAND_ALPHA 0x0001
#define RAND_ALNUM 0x0002
#define RAND_UPPER 0x0010
#define RAND_LOWER 0x0020

typedef struct {
    size_t len;
    size_t bufsize;
    size_t chunk_size;
    char ptr [0];
} str_t;

typedef struct {
    size_t len;
    size_t bufsize;
    size_t chunk_size;
    wchar_t ptr [0];
} wstr_t;

typedef struct {
    size_t len;
    char *ptr;
} strptr_t;

typedef struct {
    size_t len;
    wchar_t *ptr;
} wstrptr_t;

typedef struct {
    size_t len;
    size_t bufsize;
    size_t chunk_size;
    char *ptr;
} strbuf_t;

#define isunicode(c) (((c)&0xc0)==0xc0)

str_t *stralloc (size_t len, size_t chunk_size);
wstr_t *wstralloc (size_t len, size_t chunk_size);
wstr_t *str2wstr (const char *str, size_t str_len, size_t chunk_size);
str_t *wstr2str (const wchar_t *str, size_t str_len, size_t chunk_size);
str_t *mkstr (const char *str, size_t len, size_t chunk_size);
wstr_t *wmkstr (const wchar_t *str, size_t len, size_t chunk_size);
int strsize (str_t **str, size_t nlen, int flags);
size_t strwlen (const char *str, size_t str_len);
void strwupper (char *str, size_t str_len, locale_t locale);
void strwlower (char *str, size_t str_len, locale_t locale);
int strpad (str_t **str, size_t nlen, char filler, int flags);
int strput (str_t **str, const char *src, size_t src_len, int flags);
str_t *strput2 (str_t *str, const char *src, size_t src_len, int flags);
char *strputc (char *old_str, const char *new_str);
void strdel (str_t **str, char *pos, size_t len, int flags);
int strnadd (str_t **str, const char *src, size_t src_len);
int wstrnadd (wstr_t **str, const wchar_t *src, size_t src_len);
str_t *strconcat (size_t chunk_size, const char *arg, size_t arg_len, ...);
str_t *strev (const char *str, size_t len, size_t chunk_size);
void strntrim (char **str, size_t *str_len);
int strntok (char **str, size_t *str_len, const char *sep, size_t sep_len, strptr_t *ret);
int strepl (str_t **str, char *dst_pos, size_t dst_len, const char *src_pos, size_t src_len);
char *strnchr (const char *s, int c, size_t str_len);
char *strnrchr (const char *s, int c, size_t str_len);
char *strnstr (const char *str, size_t str_len, const char *needle, size_t n_len);
str_t *strhex (const char *prefix, const char *str, size_t str_len, size_t chunk_size);
str_t *hexstr (const char *str, size_t str_len, size_t chunk_size);
int cmpstr (const char *x, size_t x_len, const char *y, size_t y_len);
int wcmpstr (const wchar_t *x, size_t x_len, const wchar_t *y, size_t y_len);
int cmpcasestr (const char *x, size_t x_len, const char *y, size_t y_len);
int wcmpcasestr (const wchar_t *x, size_t x_len, const wchar_t *y, size_t y_len);
static inline str_t *strclone (str_t *str) { return mkstr(str->ptr, str->len, str->chunk_size); };
#define strcopy(x) strclone(x)
static inline wstr_t *wstrclone (wstr_t *str) { return wmkstr(str->ptr, str->len, str->chunk_size); };
int strbufalloc (strbuf_t *strbuf, size_t len, size_t chunk_size);
int strbufsize (strbuf_t *strbuf, size_t nlen, int flags);
int strbufput (strbuf_t *strbuf, const char *src, size_t src_len, int flags);
int strbufadd (strbuf_t *strbuf, const char *src, size_t src_len);

str_t *str_base64_encode (const char *buf, size_t bufsize, size_t chunk_size);
str_t *str_base64_decode (const char *buf, size_t bufsize, size_t chunk_size);
int strbuf_base64_encode (strbuf_t *encoded, const char *input, size_t input_len, size_t chunk_size);
int strbuf_base64_decode (strbuf_t *decoded, const char *input, size_t input_len, size_t chunk_size);
str_t *str_url_encode (const char *str, size_t str_len, size_t chunk_size);
str_t *str_url_decode (const char *str, size_t str_len, size_t chunk_size);
str_t *str_unescape (const char *src, size_t src_len, size_t chunk_size);
int strbuf_unescape (strbuf_t *strbuf, const char *src, size_t src_len);
str_t *str_escape (const char *src, size_t src_len, size_t chunk_size);
int strbuf_escape (strbuf_t *dst, const char *src, size_t src_len);
static inline size_t strset_count (void *strset) { return *((size_t*)(strset+sizeof(size_t))); }
static inline size_t strset_size (void *strset) { return *((size_t*)strset); }
void *mkstrset (size_t count, const char **strs);
void *mkstrset2 (const char **strs, size_t count, size_t *idxs);
void *strset_start (void *strset, size_t *len);
strptr_t strset_fetch (void **strset_ptr);
strptr_t strset_get (void *strset, size_t index);
int strand (char *outbuf, size_t outlen, int flags);

#endif // __LIBEX_STR_H__
