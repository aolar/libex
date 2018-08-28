/**
 * @file str.h
 * @brief string functions
 */
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
/** #LONG_FMT format for long */
#define LONG_FMT "%ld"
/** #ULONG_FMT format for unsigned long */
#define ULONG_FMT "%lu"
/** #TIME_FMT format for timne_t */
#define TIME_FMT "%lu"
/** #SIZE_FMT format for size_t */
#define SIZE_FMT "%lu"
/** #LONG_HEXFMT format for long */
#define LONG_HEXFMT "%02lX"
#else
/** #LONG_FMT format for long */
#define LONG_FMT "%lld"
/** #ULONG_FMT format for unsigned long */
#define ULONG_FMT "%llu"
/** #TIME_FMT format for timne_t */
#define TIME_FMT "%lu"
/** #SIZE_FMT format for size_t */
#define SIZE_FMT "%u"
/** #LONG_HEXFMT format for long */
#define LONG_HEXFMT "%02llX"
#endif

/** #STR_SPACES default delimiters for \b strntok */
#define STR_SPACES " \f\n\r\t\v"
#ifndef CONST_STR_LEN
/** macros for string length */
#define CONST_STR_LEN(x) x, x ? sizeof(x) - 1 : 0
/** macros for wide string legnth */
#define WCONST_STR_LEN(x) x, x ? (sizeof(x) / sizeof(wchar_t) - 1) : 0
#endif
/** NULL string */
#define CONST_STR_NULL NULL,0
/** flag for reduce memory */
#define STR_REDUCE 0x0001
/** append NULL byte to end of string */
#define STR_ADD_NULL(x) (x)->ptr[(x)->len] = '\0'
/** append NULL \b wchar_t to end of wide string */
#define WSTR_ADD_NULL(x) (x)->ptr[(x)->len] = L'\0'
/** initialize string */
#define CONST_STR_INIT(s) { .len = sizeof(s)-1, .ptr = s }
/** initialize NULL string */
#define CONST_STR_INIT_NULL { .len = 0, .ptr = NULL }

/** left paddinbg */
#define STR_LEFT 0x0001
/** center padding */
#define STR_CENTER 0x0002
/** right padding */
#define STR_RIGHT 0x0004
/** string can be UTF-8 */
#define STR_MAYBE_UTF 0x8000

/** random string as alphabetical characters */
#define RAND_ALPHA 0x0001
/** random string as alphabetical and number characters */
#define RAND_ALNUM 0x0002
/** random string will be uppercase */
#define RAND_UPPER 0x0010
/** random string will be lowercase */
#define RAND_LOWER 0x0020

/** @brief A string structure */
typedef struct {
    size_t len;                 /**< string length */
    size_t bufsize;             /**< buffer size */
    size_t chunk_size;          /**< chunk size */
    char ptr [0];               /**< string */
} str_t;

/** @brief A wide string structure */
typedef struct {
    size_t len;                 /**< string length */
    size_t bufsize;             /**< buffer size */
    size_t chunk_size;          /**< chunk size */
    wchar_t ptr [0];            /**< wide string */
} wstr_t;

/** @brief A string pointer structure */
typedef struct {
    size_t len;                 /**< string length */
    char *ptr;                  /**< string pointer */
} strptr_t;

/** @brief A wicd string pointer structure */
typedef struct {
    size_t len;                 /**< string length */
    wchar_t *ptr;               /**< string pointer */
} wstrptr_t;

/** @brief A string buffer structure */
typedef struct {
    size_t len;                 /**< string length */
    size_t bufsize;             /**< buffer size */
    size_t chunk_size;          /**< chunk size */
    char *ptr;                  /**< string pointer */
} strbuf_t;

/** @brief text unicode */
#define isunicode(c) (((c)&0xc0)==0xc0)

/** 
 * @brief create new stirng with reserved size for \b len string size
 * with \b chunk_size growing size
 * @param len reserved length
 * @param chunk_size minimal growing size
 * @return string
 */
str_t *stralloc (size_t len, size_t chunk_size);

/**
 * @brief create new wide stirng with reserved size for \b len string size
 * with \b chunk_size growing size
 * @param len reserved length
 * @param chunk_size minimal growing size
 * @return string
 */
wstr_t *wstralloc (size_t len, size_t chunk_size);

/**
 * @brief convert char string to wide string
 * @param str source string
 * @param str_len source string length
 * @param chunk_size result stirng chunk size
 * @return wide string if success, NULL if error
 */
wstr_t *str2wstr (const char *str, size_t str_len, size_t chunk_size);

/**
 * @brief convert wide string to char string
 * @param str source wide string
 * @param str_len source wide string length
 * @param chunk_size result stirng chunk size
 * @return string if success, NULL if error
 */
str_t *wstr2str (const wchar_t *str, size_t str_len, size_t chunk_size);

/**
 * @brief create string based on char buffer
 * @param str source string buffer
 * @param len length of source string buffer
 * @param chunk_size minimal growing size
 * @return string. If returns NULL then fail
 */
str_t *mkstr (const char *str, size_t len, size_t chunk_size);

/**
 * @brief create wicd string based on wide char buffer
 * @param str source wide string buffer
 * @param len length of source wide string buffer
 * @param chunk_size minimal growing size
 * @return string. If returns NULL then fail
 */
wstr_t *wmkstr (const wchar_t *str, size_t len, size_t chunk_size);

/**
 * @brief change buffer size for string. If needs then reallocated of memory
 * @param str: string
 * @param nlen - new string length
 * @param flags - if \b flags sets to STR_REDUCE and new buffer size less then old size then 
 * buffer will be decreased.
 * @return 0 is success, -1 if fail.
 */
int strsize (str_t **str, size_t nlen, int flags);

/**
 * @brief change buffer size for wide string. If needs then reallocated of memory
 * @param str wide string
 * @param str_len new wide string length
 * @return 0 is success, -1 if fail.
 */
size_t strwlen (const char *str, size_t str_len);

/**
 * @brief convert lowercase wide characters to uppercase in the wide string
 * @param str wide string
 * @param str_len string length
 * @param locale locale object 
 */
void strwupper (char *str, size_t str_len, locale_t locale);

/**
 * @brief convert uppercase wide characters to lowercase in the wide string
 * @param str wide string
 * @param str_len string length
 * @param locale locale object 
 */
void strwlower (char *str, size_t str_len, locale_t locale);

/**
 * @brief pad string to new length and fill blank spaces with needed character
 * @param str string
 * @param nlen new string length, if new string length less then source string length then do nothing
 * @param filler character for filling
 * @param flags padding type.
 * <ul>
 *  <li>STR_LEFT
 *      left padding
 *  <li>STR_RIGHT
 *      right padding
 *  <li>STR_CENTER
 *      center padding
 * </ul>
 * @return 0 if success, -1 if error
 * @section ex1 Example
 * @snippet test_str.c strpad test
 * @section ex2 Another example
 * @snippet test_str.c strpad test utf
 */
int strpad (str_t **str, size_t nlen, char filler, int flags);

/**
 * @brief set destination string from source string and reallocate memory for
 * destination string if needed
 * @param str destination string
 * @param src source string
 * @param src_len source string length
 * @param flags if equals STR_REDUCE and new string length will be less then source string then
 * it will be reallocate memory
 * @return 0 is success, -1 error.
 */
int strput (str_t **str, const char *src, size_t src_len, int flags);

/**
 * @brief set destination string from source string and reallocate memory for
 * destination string if needed
 * @param str destination string
 * @param src source string
 * @param src_len source string length
 * @param flags if equals STR_REDUCE and new string length will be less then source string then
 * it will be reallocate memory
 * @return string if success, NULL if error
 */
str_t *strput2 (str_t *str, const char *src, size_t src_len, int flags);

/**
 * @brief replace old string by new string. If \b old_str is null and \b new_str is null too then returns NULL
 * If only \b old_str is null then returns new str, if \b new_str is null returns \b old_str.
 * If reallocate memory required and this operation is fail then function returns NULL and
 * errno set ENOMEM
 * @param old_str first string
 * @param new_str second string
 * @return string if success, NULL if error.
 */
char *strputc (char *old_str, const char *new_str);

/**
 * @brief remove substring from string
 * @param str result string
 * @param pos first position
 * @param len character count for removing
 * @param flags if equals STR_REDUCE and new string length will be less then source string then
 * it will be reallocate memory
 */
void strdel (str_t **str, char *pos, size_t len, int flags);

/**
 * @brief add string to string
 * @param str result string
 * @param src source string
 * @param src_len source string legnth
 * @return 0 if success, -1 if error.
 */
int strnadd (str_t **str, const char *src, size_t src_len);

/**
 * @brief add wide string to wide string
 * @param str result string
 * @param src source string
 * @param src_len source string legnth
 * @return 0 if success, -1 if error.
 */
int wstrnadd (wstr_t **str, const wchar_t *src, size_t src_len);

/**
 * @brief concatenate strings to one string
 * @param chunk_size chunk size of string result
 * @param arg first string
 * @param arg_len length of first string
 * @return string if success, NULL if error
 */
str_t *strconcat (size_t chunk_size, const char *arg, size_t arg_len, ...);

/**
 * @brief returns string in reverse order
 * @param str source string
 * @param len source string length
 * @param chunk_size chink size of result string
 * @return string if success, NULL if error
 */
str_t *strev (const char *str, size_t len, size_t chunk_size);

/**
 * @brief remove leading and concluding tails
 * @param str string
 * @param str_len string length
 */
void strntrim (char **str, size_t *str_len);

/**
 * @brief returns next token
 * @param str string
 * @param str_len string length
 * @param sep character set as separator
 * @param sep_len length of \b sep
 * @param ret result
 * @return 0 tokens is returned and string tail is exists, 1 if it is last token, -1 if it geting token over string
 */
int strntok (char **str, size_t *str_len, const char *sep, size_t sep_len, strptr_t *ret);

/**
 * @brief replace substring in string by source string
 * @param str string
 * @param dst_pos destination position
 * @param dst_len character count of destination
 * @param src_pos source string
 * @param src_len source string length
 * @return 0 if success, -1 if error.
 * @section ex Example:
 * @snippet test_str.c strepl test
 */
int strepl (str_t **str, char *dst_pos, size_t dst_len, const char *src_pos, size_t src_len);

/**
 * @brief locate character in string
 * @param s string
 * @param str_len string length
 * @param c wanted character
 * @return pointer to found character, NULL if not found
 */
char *strnchr (const char *s, int c, size_t str_len);

/**
 * @brief locate character in string begining from last character of string
 * @param s string
 * @param str_len string length
 * @param c wanted character
 * @return pointer to found character, NULL if not found
 */
char *strnrchr (const char *s, int c, size_t str_len);

/**
 * @brief locate substring in string
 * @param str string
 * @param str_len string length
 * @param needle substring
 * @param n_len substring length
 * @return pointer to found substring, NULL if not found
 */
char *strnstr (const char *str, size_t str_len, const char *needle, size_t n_len);

/**
 * @brief convert string to HEX format
 * @param prefix result string prefix
 * @param str source string
 * @param str_len source string length
 * @param chunk_size result string chunk size
 * @return string if success or NULL if error
 */
str_t *strhex (const char *prefix, const char *str, size_t str_len, size_t chunk_size);

/**
 * @brief convert HEX string to string
 * @param str source string
 * @param str_len source string length
 * @param chunk_size result string chunk size
 * @return string if success or NULL if error
 */
str_t *hexstr (const char *str, size_t str_len, size_t chunk_size);

/**
 * @brief compare to strings
 * @param x first string
 * @param x_len first string length
 * @param y second string
 * @param y_len second string length
 * @return 1 - if x greater then y, -1 - if x less then y, 0 - if x equals y
 */
int cmpstr (const char *x, size_t x_len, const char *y, size_t y_len);

/**
 * @brief compare to wide strings
 * @param x first string
 * @param x_len first string length
 * @param y second string
 * @param y_len second string length
 * @return 1 - if x greater then y, -1 - if x less then y, 0 - if x equals y
 */
int wcmpstr (const wchar_t *x, size_t x_len, const wchar_t *y, size_t y_len);

/**
 * @brief compare to strings, ignoring the case characters
 * @param x first string
 * @param x_len first string length
 * @param y second string
 * @param y_len second string length
 * @return 1 - if x greater then y, -1 - if x less then y, 0 - if x equals y
 */
int cmpcasestr (const char *x, size_t x_len, const char *y, size_t y_len);

/**
 * @brief compare to wide strings, ignoring the case characters
 * @param x first string
 * @param x_len first string length
 * @param y second string
 * @param y_len second string length
 * @return 1 - if x greater then y, -1 - if x less then y, 0 - if x equals y
 */
int wcmpcasestr (const wchar_t *x, size_t x_len, const wchar_t *y, size_t y_len);

/**
 * @brief create new string from existing string
 * @param str source stirng
 * @return string if success, NULL if error
 */
static inline str_t *strclone (str_t *str) { return mkstr(str->ptr, str->len, str->chunk_size); };
/**
 * synonim of strclone
 */
#define strcopy(x) strclone(x)

/**
 * @brief create new wide string from existing wide string
 * @param str source stirng
 * @return string if success, NULL if error
 */
static inline wstr_t *wstrclone (wstr_t *str) { return wmkstr(str->ptr, str->len, str->chunk_size); };

/**
 * @brief create stirng buffer with reserved size for \b len string size
 * with \b chunk_size growing size
 * @param strbuf string buffer
 * @param len reserved length
 * @param chunk_size minimal growing size
 * @return 0 if success, -1 if error.
 */
int strbufalloc (strbuf_t *strbuf, size_t len, size_t chunk_size);

/**
 * @brief change string buffer size for new string buffer length
 * @param strbuf string buffer
 * @param nlen new length
 * @param flags if set flags to STR_REDUCE then decrease of a string reduced a size of memory
 * @return 0 is success, -1 if error
 */
int strbufsize (strbuf_t *strbuf, size_t nlen, int flags);

/**
 * @brief set destination string buffer from source string and reallocate memory for
 * destination string if needed
 * @param strbuf destination string
 * @param src source string
 * @param src_len source string length
 * @param flags if equals STR_REDUCE and new string length will be less then source string then
 * it will be reallocate memory
 * @return 0 is success, -1 error.
 */
int strbufput (strbuf_t *strbuf, const char *src, size_t src_len, int flags);

/**
 * @brief add string to string buffer
 * @param strbuf result string buffer
 * @param src source string
 * @param src_len source string legnth
 * @return 0 if success, -1 if error.
 */
int strbufadd (strbuf_t *strbuf, const char *src, size_t src_len);


/**
 * @brief convrt string to base64
 * @param buf source buffer
 * @param bufsize source buffer size
 * @param chunk_size result string size
 * @return string if success, NULL if memory allocation error
 */
str_t *str_base64_encode (const char *buf, size_t bufsize, size_t chunk_size);

/**
 * @brief convert base64 to string
 * @param buf input string
 * @param bufsize input string length
 * @param chunk_size result string chunk size
 * @return string if success, NULL if memory allocation error
 */
str_t *str_base64_decode (const char *buf, size_t bufsize, size_t chunk_size);

/**
 * @brief create string buffer and fill input string to base64 string buffer
 * @param encoded destination string buffer
 * @param input source string
 * @param input_len source string length
 * @param chunk_size: result string size
 * @return 0 if success, -1 if memory allocation error
 */
int strbuf_base64_encode (strbuf_t *encoded, const char *input, size_t input_len, size_t chunk_size);

/**
 * @brief create string buffer and fill it from base64 decoded string
 * @param decoded destination string buffer
 * @param input source string
 * @param input_len source string length
 * @param chunk_size: result string size
 * @return 0 if success, -1 if memory allocation error
 */
int strbuf_base64_decode (strbuf_t *decoded, const char *input, size_t input_len, size_t chunk_size);

/**
 * @brief encode url string
 * @param str source string
 * @param str_len source string length
 * @param chunk_size: result string chunk_size
 * @return string if success, NULL if error
 */
str_t *str_url_encode (const char *str, size_t str_len, size_t chunk_size);

/**
 * @brief decode url string
 * @param str source string
 * @param str_len source string length
 * @param chunk_size: result string chunk_size
 * @return string if success, NULL if error
 */
str_t *str_url_decode (const char *str, size_t str_len, size_t chunk_size);

/**
 * @brief create unescaped string
 * @param src source string
 * @param src_len source string length
 * @param chunk_size: result string chunk_size
 * @return string if success, NULL if error
 */
str_t *str_unescape (const char *src, size_t src_len, size_t chunk_size);

/**
 * @brief create unescaped string buffer
 * @param strbuf destination string buffer
 * @param src source string
 * @param src_len source string length
 * @return 0 if success, -1 if error
 */
int strbuf_unescape (strbuf_t *strbuf, const char *src, size_t src_len);

/**
 * @brief create escaped string
 * @param src source string
 * @param src_len source string length
 * @param chunk_size result string chunk_size
 * @return string if success, NULL if error
 */
str_t *str_escape (const char *src, size_t src_len, size_t chunk_size);

/**
 * @brief create escaped string buffer
 * @param dst destination string buffer
 * @param src source string
 * @param src_len source string length
 * @return 0 if success, -1 if error
 */
int strbuf_escape (strbuf_t *dst, const char *src, size_t src_len);

/**
 * @brief returns string count in memory buffer
 * @param strset strings set
 */
static inline size_t strset_count (void *strset) { return *((size_t*)(strset+sizeof(size_t))); }

/**
 * @brief returns size of memory strings buffer
 * @param strset strins set
 */
static inline size_t strset_size (void *strset) { return *((size_t*)strset); }

/**
 * @brief create string buffer set from string array
 * @param count string array count
 * @param strs source string array
 * @return string set buffer
 */
void *mkstrset (size_t count, const char **strs);

/**
 * @brief temporary undescribed
 * @param strs
 * @param count
 * @param idxs
 */
void *mkstrset2 (const char **strs, size_t count, size_t *idxs);

/**
 * @brief start for fetching string buffer set
 * @param strset string set buffer
 * @param len string set buffer length
 * @return pointer to string buffer set
 */
void *strset_start (void *strset, size_t *len);

/**
 * @brief get next string from string buffer set
 * @param strset_ptr string set
 * @return string pointer, if result is pointer is NULL and length is 0 then this is the end
 */
strptr_t strset_fetch (void **strset_ptr);

/**
 * @brief get string pointer by index
 * @param strset string set pointer
 * @param index
 * @return string pointer, if result is pointer is NULL and length is 0 then this is the end
 */
strptr_t strset_get (void *strset, size_t index);

/**
 * @brief generate random string
 * @param outbuf result string
 * @param outlen result string length
 * @param flags generate parameter flags
 * <ul>
 *      <li>RAND_ALPHA result string contains only alphabetical characters
 *      <li>RAND_ALNUM result contains alphabetical and numeric characters
 *      <li>RAND_UPPER result is uppercase characters
 *      <li>RAND_LOWER result is lowercase charactrs
 * </ul>
 * @return 0 if success, -1 if invalid \b type.
 */
int strand (char *outbuf, size_t outlen, int flags);

#endif // __LIBEX_STR_H__
