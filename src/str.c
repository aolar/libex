#include "../include/libex/str.h"

/**
 * @brief create new stirng with reserved size for @len string size
 *        with @chunk_size growing size
 * @param len reserved length
 * @param chunk_size minimal growing size
 * @return string
 */
str_t *stralloc (size_t len, size_t chunk_size) {
    size_t bufsize = (len / chunk_size) * chunk_size + chunk_size;
    if (len == bufsize) bufsize += chunk_size;
    str_t *ret = malloc(sizeof(size_t)*3 + bufsize);
    if (!ret) return NULL;
    ret->ptr[0] = '\0';
    ret->len = 0;
    ret->bufsize = bufsize;
    ret->chunk_size = chunk_size;
    return ret;
}

/**
 * wstralloc - create wide string
 * @len: reserved length
 * @chunk_size: minimal growing size
 *
 * allocate memory for new wide stirng with reserved size for @len string size
 * with @chunk_size growing size.
 *
 * Returns new string
 */
wstr_t *wstralloc (size_t len, size_t chunk_size) {
    size_t bufsize = (len / chunk_size) * chunk_size + chunk_size;
    if (len == bufsize) bufsize += chunk_size;
    wstr_t *ret = malloc(sizeof(size_t)*3 + bufsize * sizeof(wchar_t));
    if (!ret) return NULL;
    ret->ptr[0] = '\0';
    ret->len = 0;
    ret->bufsize = bufsize;
    ret->chunk_size = chunk_size;
    return ret;
}

/**
 * mkstr - create and fill string
 * @str: source string
 * @len: length of string
 * @chunk_size: minimal growing size
 *
 * create new string and fill it from source string.
 *
 * Return new string. If @len equals 0 then returns NULL
 */
str_t *mkstr (const char *str, size_t len, size_t chunk_size) {
    str_t *ret = stralloc(len, chunk_size);
    if (!ret) return NULL;
    if (0 == len) return NULL;
    ret->len = len;
    if (str && len) memcpy(ret->ptr, str, len);
    STR_ADD_NULL(ret);
    return ret;
}

/**
 * wmkstr - create and fill wide string
 * @str: source string
 * @len: length of string
 * @chunk_size: minimal growing size
 *
 * Create new wide string and fill from source string.
 *
 * Return new string. If @len equals 0 then returns NULL
 */
wstr_t *wmkstr (const wchar_t *str, size_t len, size_t chunk_size) {
    wstr_t *ret = wstralloc(len, chunk_size);
    if (!ret) return NULL;
    if (0 == len) return NULL;
    ret->len = len;
    if (str && len) memcpy(ret->ptr, str, len * sizeof(wchar_t));
    WSTR_ADD_NULL(ret);
    return ret;
}

/**
 * strsize - change length of a string
 * @str: string
 * @nlen - new length
 * @flags - if set flags to STR_REDUCE then decrease of a string reduced a size of memory
 *
 * Change length of a string.
 *
 * Returns 0 is success, -1 if reallocate of memory is fail.
 */
int strsize (str_t **str, size_t nlen, int flags) {
    str_t *s = *str;
    size_t bufsize = (nlen / s->chunk_size) * s->chunk_size + s->chunk_size;
    if (bufsize == s->bufsize) return 0;
    if (!(flags & STR_REDUCE) && bufsize < s->bufsize) return 0;
    s = realloc(s, sizeof(size_t)*3 + bufsize);
    if (!s) return -1;
    s->bufsize = bufsize;
    *str = s;
    STR_ADD_NULL(*str);
    return 0;
}

/**
 * strwlen - returns actual length of a wide string
 * @str: string buffer
 * @str_len: size of string buffer
 *
 * Returns character count for string string buffer
 */
size_t strwlen (const char *str, size_t str_len) {
    size_t actual_len = 0, i = 0;
    while (i++ < str_len)
        actual_len += (*str++ & 0xc0) != 0x80;
    return actual_len;
}

static int strlpad (str_t **str, size_t nlen, char filler) {
    int ret = strsize(str, nlen, 0);
    if (-1 == ret) return ret;
    str_t *s = *str;
    memset(s->ptr + s->len, filler, nlen - s->len);
    s->len = nlen;
    STR_ADD_NULL(*str);
    return 0;
}

static int strcpad (str_t **str, size_t nlen, char filler) {
    int ret = strsize(str, nlen, 0);
    if (-1 == ret) return ret;
    str_t *s = *str;
    size_t n = nlen / 2 - s->len / 2;
    memmove(s->ptr + n, s->ptr, s->len);
    memset(s->ptr, filler, n);
    memset(s->ptr + s->len + n, filler, nlen - s->len - n);
    s->len = nlen;
    STR_ADD_NULL(*str);
    return 0;
}

static int strrpad (str_t **str, size_t nlen, char filler) {
    int ret = strsize(str, nlen, 0);
    if (-1 == ret) return -1;
    str_t *s = *str;
    size_t n = nlen - s->len;
    memmove(s->ptr + n, s->ptr, s->len);
    memset(s->ptr, filler, n);
    s->len = nlen;
    STR_ADD_NULL(*str);
    return 0;
}

/**
 * strpad - trying to pad string
 * @str: string
 * @nlen: new string length
 * @filler: character for filling blank space
 * @flags: flags is same as flags in strsize function
 *
 * Change @str size if needed, pad string and fills blank spsce by @filler
 * Flags can get the next values:
 * STR_LEFT - left padding,
 * STR_CENTER - center padding,
 * STR_RIGHT - right padding.
 *
 * Returns 0 is success, -1 if memory allocation is fail.
 */
int strpad (str_t **str, size_t nlen, char filler, int flags) {
    str_t *s = *str;
    if ((flags & STR_MAYBE_UTF)) {
        size_t cnt = strwlen(s->ptr, s->len);
        nlen += s->len - cnt;
    }
    if ((*str)->len >= nlen) return 0;
    if ((flags & STR_LEFT)) strlpad(str, nlen, filler); else
    if ((flags & STR_CENTER)) strcpad(str, nlen, filler); else
        strrpad(str, nlen, filler);
    STR_ADD_NULL(*str);
    return 0;
}

/**
 * strput - assign string from existing string
 * @str: destination string
 * @src: source string
 * @src_len: source string length
 * @flags: flags is same as flags in strsize function
 *
 * Returns 0 is success, -1 if memory alocation is fail.
 */
int strput (str_t **str, const char *src, size_t src_len, int flags) {
    if (-1 == strsize(str, src_len, flags)) return -1;
    (*str)->len = src_len;
    memcpy((*str)->ptr, src, src_len);
    STR_ADD_NULL(*str);
    return 0;
}

/**
 * strput2 - assign string from existing string
 * @str: destination string
 * @src: source string
 * @src_len: source string length
 * @flags: flags is same as flags in strsize function
 *
 * Returns string if success, NULL if memory allocation is fail.
 */
str_t *strput2 (str_t *str, const char *src, size_t src_len, int flags) {
    if (-1 == strsize(&str, src_len, flags)) return NULL;
    str->len = src_len;
    memcpy(str->ptr, src, src_len);
    STR_ADD_NULL(str);
    return str;
}

/**
 * strputc - replace old string by new string
 * @old_str: first string
 * @new_str: second string
 *
 * Returns string. If @old_str is null and @new_str is null too then returns NULL
 * If only old_str is null then returns new str, if new_str is null returns @old_str.
 * If reallocate memory required and this operation is fail then function returns NULL and
 * errno set ENOMEM
 */
char *strputc (char *old_str, const char *new_str) {
    char *res = old_str;
    if (!res) {
        if (new_str)
            return strdup(new_str);
        else
            return NULL;
    } else
    if (!new_str)
        return res;
    size_t old_len = strlen(old_str), new_len = strlen(new_str);
    if (new_len > old_len) {
        char *p = realloc(res, new_len+1);
        if (!p)
            return NULL;
        res = p;
    }
    //strncpy(res, new_str, new_len);
    strcpy(res, new_str); // FIXME ?
    return res;
}

/**
 * strdel - remove substring from string
 * @str: string
 * @pos: first position
 * @len: character count for removing
 * @flags: same as flags n strsize() function
 *
 */
void strdel (str_t **str, char *pos, size_t len, int flags) {
    str_t *s = *str;
    char *e = s->ptr + s->len, *epos = pos + len;
    if (pos > e) return;
    if (epos >= e) {
        s->len = (uintptr_t)pos - (uintptr_t)s->ptr;
        strsize(str, s->len, flags);
        return;
    }
    s->len -= len;
    memmove(pos, epos, (uintptr_t)e - (uintptr_t)epos);
    strsize(str, s->len, flags);
    STR_ADD_NULL(*str);
}

/**
 * strntrim - remove leading and concluding tails
 * @str: string
 * @str_len string length
 *
 */
void strntrim (char **str, size_t *str_len) {
    if (*str_len == 0) return;
    size_t len = *str_len;
    char *p = *str, *e = p + len - 1;
    while (p <= e && isspace(*p)) ++p;
    while (p <= e && isspace(*e)) --e;
    *str = p;
    if (!isspace(*e)) ++e;
    *str_len = (uintptr_t)e - (uintptr_t)p;
}

/**
 * strntok - returns next token
 * @str: string
 * @str_len: string length
 * @sep: character set as separator
 * @sep_len: length of @sep
 * @ret: result
 *
 * Returns token from @str and change pointer @str for next operation.
 */
int strntok (char **str, size_t *str_len, const char *sep, size_t sep_len, strptr_t *ret) {
    char *p = *str, *e = *str + *str_len, *q;
    if (p >= e)
        return -1;
    while (p < e && strnchr(sep, *p, sep_len))
        ++p;
    q = p;
    while (q < e && !strnchr(sep, *q, sep_len))
        ++q;
    ret->ptr = p;
    ret->len = (uintptr_t)q - (uintptr_t)p;
    ++q;
    if (ret->len > 0)
        strntrim(&ret->ptr, &ret->len);
    while (q < e && strchr(sep, *q))
        ++q;
    *str = q;
    *str_len = e < q ? 0 : (uintptr_t)e - (uintptr_t)q;
    return 0;
}

/**
 * strepl - replace substring in string by source string
 * @str: string
 * @dst_pos: destination position
 * @dst_len: character count of destination
 * @src_pos: source string
 * @src_len: source string length
 *
 * Returns 0 if success. If it required reallocated memory then set errno to ERANGE. If this operation is fail thne returns -1.
 */
int strepl (str_t **str, char *dst_pos, size_t dst_len, const char *src_pos, size_t src_len) {
    str_t *s = *str;
    size_t nstr_len = s->len;
    errno = 0;
    if (dst_len < src_len) {
        size_t dv = src_len - dst_len;
        if (s->bufsize <= s->len + dv) {
            size_t nbufsize = ((1 + s->len + dv) / s->chunk_size) * s->chunk_size + s->chunk_size, dst = (uintptr_t)dst_pos - (uintptr_t)s->ptr;
            str_t *nstr = realloc(s, nbufsize + sizeof(size_t) * 3);
            errno = ERANGE;
            if (!nstr) return -1;
            s = *str = nstr;
            s->bufsize = nbufsize;
            dst_pos = nstr->ptr + dst;
        }
        nstr_len += dv;
    } else
        nstr_len -= dst_len - src_len;
    memmove(dst_pos + src_len, dst_pos + dst_len, s->len - ((uintptr_t)dst_pos - (uintptr_t)s->ptr + dst_len));
    memcpy(dst_pos, src_pos, src_len);
    s->len = nstr_len;
    STR_ADD_NULL(*str);
    return 0;
}

/**
 * strnchr - locate character in string
 * @s: string
 * @str_len: string length
 * @c: wanted character
 *
 * This function same as strchr from libc, buf fin of function controlled of string length
 */
char *strnchr (const char *s, int c, size_t str_len) {
    const char *e = s + str_len;
    while (s < e && *s != c) ++s;
    if (s >= e) return NULL;
    return (char*)s;
}

/**
 * strnrchr - locate character in string
 * @s: string
 * @str_len: string length
 * @c: wanted character
 *
 * This function same as strrchr from libc, buf fin of function controlled of string length
 */
char *strnrchr (const char *s, int c, size_t str_len) {
    const char *e = s + str_len - 1;
    while (s <= e && *e != c) --e;
    if (e < s) return NULL;
    return (char*)e;
}

/**
 * strnstr - locate a substring
 * @str: string
 * @str_len: string length
 * @needle: is the string haystack
 * @length of is the string haystack
 *
 * This function same as strstr from libc, buf fin of function controlled by string length and @needle length
 */
char *strnstr (const char *str, size_t str_len, const char *needle, size_t n_len) {
    const char *p, *pe;
    size_t len;
    if (NULL == needle || 0 == n_len || str_len < n_len) return NULL;
    len = str_len - n_len;
    p = str, pe = p + len + 1;
    while (p < pe) {
        const char *q = needle, *pne, *pf;
        while (p < pe && *p != *q) ++p;
        if (p == pe) break;
        pf = p;
        pne = p + n_len;
        while (p < pne && *q == *p) { ++p; ++q; }
        if (p == pne) {
            return (char*)pf;
        }
    }
    return NULL;
}

/**
 * strnadd - add string to string
 * @str: string
 * @src: source string
 * @src_len: source string legnth
 *
 * Returns 0 if success, -1 if reallocate memory is required and it finished by fail.
 */
int strnadd (str_t **str, const char *src, size_t src_len) {
    str_t *s = *str;
    size_t nstr_len = s->len + src_len;
    if (nstr_len >= s->bufsize) {
        size_t nbufsize = (nstr_len / s->chunk_size) * s->chunk_size + s->chunk_size;
        str_t *nstr = realloc(s, sizeof(size_t)*3 + nbufsize);
        if (!nstr) return -1;
        s = *str = nstr;
        s->bufsize = nbufsize;
    }
    memcpy(s->ptr + s->len, src, src_len);
    s->len = nstr_len;
    STR_ADD_NULL(*str);
    return 0;
}

/**
 * wstrnadd - add wie string to wide string
 * @str: string
 * @src: source string
 * @src_len: source string legnth
 *
 * Returns 0 if success, -1 if reallocate memory is required and it finished by fail.
 */
int wstrnadd (wstr_t **str, const wchar_t *src, size_t src_len) {
    wstr_t *s = *str;
    size_t nstr_len = s->len + src_len;
    if (nstr_len >= s->bufsize) {
        size_t nbufsize = (nstr_len / s->chunk_size) * s->chunk_size + s->chunk_size;
        wstr_t *nstr = realloc(s, sizeof(size_t)*3 + nbufsize * sizeof(wchar_t));
        if (!nstr) return -1;
        s = *str = nstr;
        s->bufsize = nbufsize;
    }
    memcpy(s->ptr + s->len * sizeof(wchar_t), src, src_len * sizeof(wchar_t));
    s->len = nstr_len;
    WSTR_ADD_NULL(*str);
    return 0;
}

/**
 * strconcat - concatenate strings
 * @chunc_size: minimal size for reallocating memory for string in future
 * @arg: arguments
 *
 * Returns string if success and NULL if memory allocation is fail
 */
str_t *strconcat (size_t chunk_size, const char *arg, size_t arg_len, ...) {
    size_t start_len = arg_len;
    va_list ap;
    if (!arg || !arg_len)
        return NULL;
    va_start(ap, arg_len);
    while (1) {
        const char *s = va_arg(ap, const char*);
        if (!s)
            break;
        size_t n = va_arg(ap, size_t);
        start_len += n;
    }
    va_end(ap);
    str_t *res = stralloc(start_len, chunk_size);
    if (!res) return NULL;
    if (-1 == strnadd(&res, arg, arg_len)) {
        free(res);
        return NULL;
    }
    va_start(ap, arg_len);
    while (1) {
        const char *s = va_arg(ap, const char*);
        if (!s)
            break;
        size_t n = va_arg(ap, size_t);
        if (!n)
            break;
        if (-1 == strnadd(&res, s, n)) {
            free(res);
            return NULL;
        }
    }
    va_end(ap);
    return res;
}

/**
 * strev - reverse string
 * @str: source string
 * @len: source string length
 * @chunk_size: chunk size of result string
 *
 * Returs string if success or NULL if can't allocate memory
 */
str_t *strev (const char *str, size_t len, size_t chunk_size) {
    str_t *ret = stralloc(len, chunk_size);
    char *p = ret->ptr, *q = (char*)str + len - 1;
    while (q >= str) {
        *p++ = *q--;
    }
    ret->len = len;
    STR_ADD_NULL(ret);
    return ret;
}

/**
 * strhex - convert string to HEX format
 * @prifix: result string prefix
 * @str: source string
 * @str_len: source string length
 * @chunk_size: result string chunk size
 *
 * Returs string if success or NULL if can't allocate memory
 */
str_t *strhex (const char *prefix, const char *str, size_t str_len, size_t chunk_size) {
    char buf [8];
    size_t prefix_len = prefix ? strlen(prefix) : 0;
    str_t *result = stralloc(2 * str_len + prefix_len, chunk_size);
    if (prefix && prefix_len)
        strnadd(&result, prefix, prefix_len);
    for (size_t i = 0; i < str_len; ++i) {
        snprintf(buf, sizeof buf, "%02x", str[i]);
        strnadd(&result, buf, 2);
    }
    return result;
}

/**
 * hexstr - convert HEX string to string
 * @str: source string
 * @str_len: source string length
 * @chunk_size: result string chunk size
 *
 * Returs string if success or NULL if can't allocate memory
 */
str_t *hexstr (const char *str, size_t str_len, size_t chunk_size) {
    char buf [3] = {0,0,0}, *tail;
    const char *p = str, *e = p + str_len;
    str_t *res = stralloc(str_len / 2, chunk_size);
    errno = 0;
    while (p < e) {
        buf[0] = *p++;
        if (p == e) {
            errno = ERANGE;
            return res;
        }
        buf[1] = *p++;
        buf[2] = '\0';
        int n = strtol(buf, &tail, 16);
        if ('\0' != *tail || ERANGE == errno) {
            errno = ERANGE;
            return res;
        }
        buf[0] = n;
        buf[1] = '\0';
        strnadd(&res, buf, 1);
    }
    return res;
}

/**
 * cmpstr - compare to strings
 * @x: first string
 * @x_len: first string length
 * @y: second string
 * @y_len: second string length
 *
 * Returns 1 - if x greater then y, -1 - if x less then y, 0 - if x equals y
 */
int cmpstr (const char *x, size_t x_len, const char *y, size_t y_len) {
    if (x_len < y_len) return -1;
    if (x_len > y_len) return 1;
    return strncmp(x, y, x_len);
}

/**
 * wcmpstr - compare to wide strings
 * @x: first string
 * @x_len: first string length
 * @y: second string
 * @y_len: second string length
 *
 * Returns 1 - if x greater then y, -1 - if x less then y, 0 - if x equals y
 */
int wcmpstr (const wchar_t *x, size_t x_len, const wchar_t *y, size_t y_len) {
    if (x_len < y_len) return -1;
    if (x_len > y_len) return 1;
    return wcsncmp(x, y, x_len);
}

/**
 * cmpcasestr - compare to strings, ignoring the case of characters
 * @x: first string
 * @x_len: first string length
 * @y: second string
 * @y_len: second string length
 *
 * Returns 1 - if x greater then y, -1 - if x less then y, 0 - if x equals y
 */
int cmpcasestr (const char *x, size_t x_len, const char *y, size_t y_len) {
    if (x_len < y_len) return -1;
    if (x_len > y_len) return 1;
    return strncasecmp(x, y, x_len);
}

/**
 * wcmpcasestr - compare to wide strings, ignoring the case of characters
 * @x: first string
 * @x_len: first string length
 * @y: second string
 * @y_len: second string length
 *
 * Returns 1 - if x greater then y, -1 - if x less then y, 0 - if x equals y
 */
int wcmpcasestr (const wchar_t *x, size_t x_len, const wchar_t *y, size_t y_len) {
    if (x_len < y_len) return -1;
    if (x_len > y_len) return 1;
    return wcsncasecmp(x, y, x_len);
}

/**
 * strbufalloc - create string buffer
 * @strbuf: string buffer
 * @len: reserved length
 * @chunk_size: minimal growing size
 *
 * allocate memory for new stirng buffer with reserved size for @len string size
 * with @chunk_size growing size
 *
 * Returns new string
 */
int strbufalloc (strbuf_t *strbuf, size_t len, size_t chunk_size) {
    size_t bufsize = (len / chunk_size) * chunk_size + chunk_size;
    if (len == bufsize) bufsize += chunk_size;
    char *buf = malloc(bufsize);
    if (!buf) return -1;
    strbuf->ptr = buf;
    strbuf->ptr[0] = '\0';
    strbuf->len = 0;
    strbuf->bufsize = bufsize;
    strbuf->chunk_size = chunk_size;
    return 0;
}

/**
 * strbufsize - change length of a string buffer
 * @strbuf: string buffer
 * @nlen - new length
 * @flags - if set flags to STR_REDUCE then decrease of a string reduced a size of memory
 *
 * Change length of a string.
 *
 * Returns 0 is success, -1 if reallocate of memory is fail.
 */
int strbufsize (strbuf_t *strbuf, size_t nlen, int flags) {
    char *buf = strbuf->ptr;
    size_t bufsize = (nlen / strbuf->chunk_size) * strbuf->chunk_size + strbuf->chunk_size;
    if (bufsize == strbuf->bufsize) return 0;
    if (!(flags & STR_REDUCE) && bufsize < strbuf->bufsize) return 0;
    buf = realloc(buf, bufsize);
    if (!buf) return -1;
    strbuf->bufsize = bufsize;
    strbuf->ptr = buf;
    if (nlen < strbuf->len) strbuf->len = nlen;
    return 0;
}

/**
 * strbufput - assign string from existing string
 * @strbuf: destination string buffer
 * @src: source string
 * @src_len: source string length
 * @flags: flags is same as flags in strsize function
 *
 * Returns 0 is success, -1 if memory alocation is fail.
 */
int strbufput (strbuf_t *strbuf, const char *src, size_t src_len, int flags) {
    if (-1 == strbufsize(strbuf, src_len, flags)) return -1;
    strbuf->len = src_len;
    memcpy(strbuf->ptr, src, src_len);
    return 0;
}

/**
 * strbufadd - add string to string buffer
 * @strbuf: string buffer
 * @src: source string
 * @src_len: source string legnth
 *
 * Returns 0 if success, -1 if reallocate memory is required and it finished by fail.
 */
int strbufadd (strbuf_t *strbuf, const char *src, size_t src_len) {
    char *buf = strbuf->ptr;
    size_t nstr_len = strbuf->len + src_len;
    if (nstr_len >= strbuf->bufsize) {
        size_t nbufsize = (nstr_len / strbuf->chunk_size) * strbuf->chunk_size + strbuf->chunk_size;
        buf = realloc(buf, nbufsize);
        if (!buf) return -1;
        strbuf->ptr = buf;
        strbuf->bufsize = nbufsize;
    }
    memcpy(strbuf->ptr + strbuf->len, src, src_len);
    strbuf->len = nstr_len;
    return 0;
}

static char encoding_table [] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
static char decoding_table [] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x3E,0x00,0x00,0x00,0x3F,
    0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,
    0x0F,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x00,0x00,0x00,0x00,0x00,
    0x00,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,
    0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 };
static int mod_table[] = {0, 2, 1};

#define ENCODED_LEN(X) 4 * ((X + 2) / 3)

/**
 * base64_encode - convert string to base64
 * @data: input data
 * @input_length: input data size
 * @encoded_data: output string
 * @output_length: output string length
 *
 */
static void base64_encode(const unsigned char *data, size_t input_length, char *encoded_data, size_t output_length) {
    for (int i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    for (int i = 0; i < mod_table[input_length % 3]; i++)
        encoded_data[output_length - 1 - i] = '=';
}

/**
 * str_base64_encode - convrt string to base64
 * @bug: source buffer
 * @bufsize: source buffer size
 * @chunk_size: result string size
 *
 * Returns string if success, NULL if memory allocation error
 */
str_t *str_base64_encode (const char *buf, size_t bufsize, size_t chunk_size) {
    size_t len = ENCODED_LEN(bufsize);
    str_t *res = stralloc(ENCODED_LEN(bufsize), chunk_size);
    memset(res->ptr, 0, res->bufsize);
    base64_encode((const unsigned char*)buf, bufsize, res->ptr, len);
    res->len = len;
    return res;
}

/**
 * strbuf_base64_encode - convert string to base64
 * @encoded: result string buffer
 * @input: source data
 * @input_len: source data length
 * @chunk_size: result string buffer chunk size
 *
 * Returns 0 if success, -1 if memory allocation error
 */
int strbuf_base64_encode (strbuf_t *encoded, const char *input, size_t input_len, size_t chunk_size) {
    size_t len = ENCODED_LEN(input_len);
    if (-1 == strbufalloc(encoded, len, chunk_size)) return -1;
    memset(encoded->ptr, 0, encoded->bufsize);
    base64_encode((const unsigned char*)input, input_len, encoded->ptr, len);
    encoded->len = len;
    return 0;
}

#define DECODED_LEN(X) X / 4 * 3;

/**
 * base64_decode - convert base64 to string
 * @data: input data
 * @input_length: input data length
 * @decoded_data: result data
 * @output_length: result data length
 *
 */
static void base64_decode(const char *data, size_t input_length, unsigned char *decoded_data, size_t output_length) {
    for (int i = 0, j = 0; i < input_length;) {
        uint32_t sextet_a = data[i] == '=' ? 0 & i++ : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_b = data[i] == '=' ? 0 & i++ : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_c = data[i] == '=' ? 0 & i++ : decoding_table[(unsigned char)data[i++]];
        uint32_t sextet_d = data[i] == '=' ? 0 & i++ : decoding_table[(unsigned char)data[i++]];
        uint32_t triple = (sextet_a << 3 * 6) + (sextet_b << 2 * 6) + (sextet_c << 1 * 6) + (sextet_d << 0 * 6);
        if (j < output_length) decoded_data[j++] = (triple >> 2 * 8) & 0xFF;
        if (j < output_length) decoded_data[j++] = (triple >> 1 * 8) & 0xFF;
        if (j < output_length) decoded_data[j++] = (triple >> 0 * 8) & 0xFF;
    }
}

/**
 * str_base64_decode - convert base64 to string
 * @buf: input string
 * @bufsize: input string length
 * @chunk_size: result string chunk size
 *
 * Returns string if success, NULL if memory allocation error
 */
str_t *str_base64_decode (const char *buf, size_t bufsize, size_t chunk_size) {
    if (bufsize % 4 != 0) return NULL;
    size_t len = DECODED_LEN(bufsize);
    if (buf[bufsize-1] == '=') len--;
    if (buf[bufsize-2] == '=') len--;
    str_t *res = stralloc(len, chunk_size);
    memset(res->ptr, 0, res->bufsize);
    base64_decode(buf, bufsize, (unsigned char*)res->ptr, len);
    res->len = len;
    return res;
}


/**
 * strbuf_base64_decode - convert base64 to string
 * @strbuf: result string buffer
 * @input: input string
 * @input_len: input string length
 * @chunk_size: result string buffer chunk size
 *
 * Returns 0 success, -1 if memory allocation error
 */
int strbuf_base64_decode (strbuf_t *decoded, const char *input, size_t input_len, size_t chunk_size) {
    if (input_len % 4 != 0) return -1;
    size_t len = DECODED_LEN(input_len);
    if (input[input_len-1] == '=') len--;
    if (input[input_len-1] == '=') len--;
    if (-1 == strbufalloc(decoded, len, chunk_size)) return -1;
    memset(decoded->ptr, 0, decoded->bufsize);
    base64_decode(input, input_len, (unsigned char*)decoded->ptr, len);
    decoded->len = len;
    return 0;
}


/**
 * str_url_encode - encode url
 * @str: input string
 * @str_len: input string length
 * @chunk_size: result string chunk_size
 *
 * Returns string if success, NULL if error
 */
str_t *str_url_encode (const char *str, size_t str_len, size_t chunk_size) {
    str_t *ret = stralloc(str_len, chunk_size);
    for (size_t i = 0; i < str_len; ++i) {
        unsigned char c = str[i];
        if (!isalnum(c) && '.' != c && '-' != c && '_' != c && '~' != c) {
            if (-1 == strsize(&ret, ret->len + 4, 0)) goto err;
            ret->len += snprintf(ret->ptr + ret->len, 4, "%%%02X", c);
        } else
            if (-1 == strnadd(&ret, (char*)&c, sizeof(char))) goto err;
    }
    return ret;
    err:
    free(ret);
    return NULL;
}

/**
 * str_url_encode - encode url
   @ecoded: result string buffer
 * @str: input string
 * @str_len: input string length
 * @chunk_size: result string chunk_size
 *
 * Returns 0 if success, -1 if error
 */
int strbuf_url_encode (strbuf_t *encoded, const char *str, size_t str_len, size_t chunk_size) {
    for (size_t i = 0; i < str_len; ++i) {
        unsigned char c = str[i];
        if (!isalnum(c) && '.' != c && '-' != c && '_' != c && '~' != c) {
            if (-1 == strbufsize(encoded, encoded->len + 4, 0)) return -1;
            encoded->len += snprintf(encoded->ptr + encoded->len, 4, "%%%02X", c);
        } else
            if (-1 == strbufadd(encoded, (char*)&c, sizeof(char))) return -1;
    }
    return 0;
}

/**
 * str_url_decode - decode url
 * @str: input string
 * @str_len: input string length
 * @chunk_size: result string chunk_size
 *
 * Returns string if success, NULL if error
 */
str_t *str_url_decode (const char *str, size_t str_len, size_t chunk_size) {
    str_t *ret = stralloc(str_len * 1.5, chunk_size);
    char num [] = "0x0__";
    for (size_t i = 0; i < str_len; ++i) {
        if (str[i] == '%') {
            char c, *tail;
            num[3] = str[++i];
            num[4] = str[++i];
            c = strtol(num, &tail, 16);
            if (*tail || errno == ERANGE) goto err;
            if (-1 == strnadd(&ret, &c, sizeof(char))) goto err;
        } else {
            if (-1 == strnadd(&ret, &str[i], sizeof(char))) goto err;
        }
    }
    return ret;
    err:
    free(ret);
    return NULL;
}

/**
 * strbuf_url_decode - decode url
 * @decoded: result string buffer
 * @str: input string
 * @str_len: input string length
 * @chunk_size: result string chunk_size
 *
 * Returns string if success, NULL if error
 */
int strbuf_url_decode (strbuf_t *decoded, const char *str, size_t str_len, size_t chunk_size) {
    char num [] = "0x0__";
    for (size_t i = 0; i < str_len; ++i) {
        if (str[i] == '%') {
            char c, *tail;
            num[3] = str[++i];
            num[4] = str[++i];
            c = strtol(num, &tail, 16);
            if (*tail || errno == ERANGE) return -1;
            if (-1 == strbufadd(decoded, &c, sizeof(char))) return -1;
        } else {
            if (-1 == strbufadd(decoded, &str[i], sizeof(char))) return -1;
        }
    }
    return 0;
}

static int is_octal_digit (char c) {
    return c >= '0' && 'c' <= '7';
}

static int is_hex_digit(char c) {
    return  (c >= '0' && c <= '9') ||
            (c >= 'A' && c <= 'F') ||
            (c >= 'a' && c <= 'f');
}

static int u8_read_esc_seq (const char *str, uint32_t *dst) {
    char digs[9] = "\0\0\0\0\0\0\0\0";
    int dno = 0, i = 1;
    uint32_t ch = (uint32_t)*str;
    if (*str == 'n') ch = L'\n'; else
    if (*str == 'r') ch = L'\r'; else
    if (*str == 't') ch = L'\t'; else
    if (*str == 'b') ch = L'\b'; else
    if (*str == 'f') ch = L'\f'; else
    if (*str == 'v') ch = L'\v'; else
    if (*str == 'a') ch = L'\a'; else
    if (is_octal_digit(*str)) {
        i = 0;
        do {
            digs[dno++] = str[i++];
        } while (is_octal_digit(str[i]) && dno < 3);
        ch = strtol(digs, NULL, 8);
    } else
    if (*str == 'x') {
        while (is_hex_digit(str[i]) && dno < 2)
            digs[dno++] = str[i++];
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    } else
    if (*str == 'u') {
        while (is_hex_digit(str[i]) && dno < 4)
            digs[dno++] = str[i++];
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    } else
    if (*str == 'U') {
        while (is_hex_digit(str[i]) && dno < 8)
            digs[dno++] = str[i++];
        if (dno > 0)
            ch = strtol(digs, NULL, 16);
    }
    *dst = ch;
    return i;
}

static int u8_wc2utf8 (char *dst, uint32_t ch) {
    if (ch < 0x80) {
        dst[0] = (char)ch;
        return 1;
    }
    if (ch < 0x800) {
        dst[0] = (ch>>6) | 0xC0;
        dst[1] = (ch & 0x3F) | 0x80;
        return 2;
    }
    if (ch < 0x10000) {
        dst[0] = (ch>>12) | 0xE0;
        dst[1] = ((ch>>6) & 0x3F) | 0x80;
        dst[2] = (ch & 0x3F) | 0x80;
        return 3;
    }
    if (ch < 0x110000) {
        dst[0] = (ch>>18) | 0xF0;
        dst[1] = ((ch>>12) & 0x3F) | 0x80;
        dst[2] = ((ch>>6) & 0x3F) | 0x80;
        dst[3] = (ch & 0x3F) | 0x80;
        return 4;
    }
    return 0;
}

static size_t utf8size (const wchar_t *str, size_t str_len) {
    size_t r = 0;
    for (int i = 0; i < str_len; ++i) {
        wchar_t c = str[i];
        if (c < 0x80) r += 1;
        if (c < 0x800) r += 2;
        if (c < 0x10000) r += 3;
        if (c < 0x110000) r += 4;
    }
    return r;
}

/**
 * str_unescape - unescape source string
 * @src: source string
 * @src_len: source string length
 * @chunk_size: result string chunk_size
 *
 * Returns string if success, NULL if error
 */
str_t *str_unescape (const char *src, size_t src_len, size_t chunk_size) {
    str_t *ret = stralloc(src_len, chunk_size);
    const char *e = src + src_len;
    char *buf = ret->ptr, *be = buf + ret->bufsize, temp[4];
    int amt;
    uint32_t ch;
    while (src < e) {
        if (*src == '\\')
            amt = u8_read_esc_seq(++src, &ch);
        else {
            ch = (uint32_t)*src;
            amt = 1;
        }
        src += amt;
        amt = u8_wc2utf8(temp, ch);
        if (amt > (uintptr_t)be - (uintptr_t)buf) {
            if (-1 == strsize(&ret, ret->len + ret->chunk_size, 0)) {
                free(ret);
                return NULL;
            }
            buf = ret->ptr + ret->len;
            be = ret->ptr + ret->bufsize;
        }
        memcpy(buf, temp, amt);
        ret->len += amt;
        buf += amt;
    }
    STR_ADD_NULL(ret);
    return ret;
}

/**
 * strbuf_unescape - unescape source string
 * @strbuf: result string buffer
 * @src: source string
 * @src_len: source string length
 *
 * Returns 0 if success, -1 if error
 */
int strbuf_unescape (strbuf_t *strbuf, const char *src, size_t src_len) {
    const char *e = src + src_len;
    char *buf = strbuf->ptr, *be = buf + strbuf->bufsize, temp[4];
    int amt;
    uint32_t ch;
    while (src < e) {
        if (*src == '\\')
            amt = u8_read_esc_seq(++src, &ch);
        else {
            ch = (uint32_t)*src;
            amt = 1;
        }
        src += amt;
        amt = u8_wc2utf8(temp, ch);
        if (amt > (uintptr_t)be - (uintptr_t)buf) {
            if (-1 == strbufsize(strbuf, strbuf->len + strbuf->chunk_size, 0))
                return -1;
            buf = strbuf->ptr + strbuf->len;
            be = strbuf->ptr + strbuf->bufsize;
        }
        memcpy(buf, temp, amt);
        strbuf->len += amt;
        buf += amt;
    }
    strbuf->ptr[strbuf->len] = '\0';
    return 0;
}

typedef unsigned char utf8_t;

static int utf8_decode (const char *str, size_t *i) {
    const utf8_t *s = (const utf8_t*)str;
    int u = *s, l = 1;
    if (isunicode(u)) {
        int a = (u&0x20)? ((u&0x10)? ((u&0x08)? ((u&0x04)? 6 : 5) : 4) : 3) : 2;
        if (a<6 || !(u&0x02)) {
            int b;
            u = ((u<<(a+1))&0xff)>>(a+1);
            for (b = 1; b < a; ++b)
                u = (u<<6)|(s[l++]&0x3f);
        }
    }
    if (i)*i += l;
    return u;
}

/**
 * str_escape - convert string to escaped string
 * @src: source stirng
 * @src_len: source string length
 * @chunk_size: result string chunk size
 *
 * Returns string if success, NULL if error
 */
str_t *str_escape (const char *src, size_t src_len, size_t chunk_size) {
    str_t *ret = stralloc(src_len, chunk_size);
    int i;
    for (i = 0; i < src_len && src[i] != '\0'; ) {
        char c = src[i];
        if (!isunicode(c)) {
            switch (c) {
                case '\"': strnadd(&ret, CONST_STR_LEN("\\\"")); break;
                case '\\': strnadd(&ret, CONST_STR_LEN("\\")); break;
                case '\b': strnadd(&ret, CONST_STR_LEN("\\b")); break;
                case '\f': strnadd(&ret, CONST_STR_LEN("\\f")); break;
                case '\n': strnadd(&ret, CONST_STR_LEN("\\n")); break;
                case '\r': strnadd(&ret, CONST_STR_LEN("\\r")); break;
                case '\t': strnadd(&ret, CONST_STR_LEN("\\t")); break;
                default: strnadd(&ret, &c, sizeof(char)); break;
            }
            i++;
        } else {
            char buf [8];
            size_t l = 0;
            int z = utf8_decode(&src[i], &l);
            snprintf(buf, sizeof buf, "\\u%04x", z);
            if (-1 == strnadd(&ret, buf, strlen(buf))) {
                free(ret);
                return NULL;
            } i += l;
        }
    }
    return ret;
}

typedef wint_t (*ul_conv_h) (wint_t, locale_t);
static void str_ul_conv (char *str, size_t str_len, locale_t locale, ul_conv_h ul_conv) {
    char *p = str;
    size_t i = 0;
    while (i < str_len) {
        size_t clen = 0, plen;
        wint_t wi = utf8_decode(&str[i], &clen);
        i += clen;
        wi = ul_conv(wi, locale);
        plen = u8_wc2utf8(p, wi);
        p += plen;
    }
}

/**
 * strwupper - convert lowercase wide characters to uppercase
 * @str: source string
 * @str_len: source string length
 * @locale: locale object, see duplocale(3)
 *
 */
inline void strwupper (char *str, size_t str_len, locale_t locale) {
    str_ul_conv(str, str_len, locale, towupper_l);
}

/**
 * strwupper - convert uppercase wide characters to lowercase
 * @str: source string
 * @str_len: source string length
 * @locale: locale object, see duplocale(3)
 *
 */
inline void strwlower (char *str, size_t str_len, locale_t locale) {
    str_ul_conv(str, str_len, locale, towlower_l);
}

/**
 * str2wstr - convert string to wide string
 * @str: source string
 * @str_len: source string length
 * @chunk_size; result stirng chunk size
 *
 * Returns wide string if success, NULL if error
 */
wstr_t *str2wstr (const char *str, size_t str_len, size_t chunk_size) {
    wstr_t *ret = wstralloc(str_len, chunk_size);
    size_t i = 0, j = 0;
    while (i < str_len) {
        size_t clen = 0;
        wint_t wi = utf8_decode(&str[i], &clen);
        i += clen;
        ret->ptr[j++] = wi;
    }
    ret->len = str_len;
    return ret;
}

/**
 * wstr2str - convert wide string to string
 * @str: source wide string
 * @str_len: source wide string length
 * @chunk_size; result stirng chunk size
 *
 * Returns string if success, NULL if error
 */
str_t *wstr2str (const wchar_t *str, size_t str_len, size_t chunk_size) {
    size_t i = 0;
    str_t *ret = stralloc(utf8size(str, str_len), chunk_size);
    char *p = ret->ptr;
    while (i < str_len) {
        size_t plen = u8_wc2utf8(p, str[i]);
        p += plen;
    }
    return ret;
}

/**
 * strbuf_escape - convert string to escaped string buffer
 * @dst: result string buffer
 * @src: source stirng
 * @src_len: source string length
 *
 * Returns string if success, NULL if error
 */
int strbuf_escape (strbuf_t *dst, const char *src, size_t src_len) {
    int i;
    for (i = 0; i < src_len && src[i] != '\0'; ) {
        char c = src[i];
        if (!isunicode(c)) {
            switch (c) {
                case '\"': strbufadd(dst, CONST_STR_LEN("\\\"")); break;
                case '\\': strbufadd(dst, CONST_STR_LEN("\\")); break;
                case '\b': strbufadd(dst, CONST_STR_LEN("\\b")); break;
                case '\f': strbufadd(dst, CONST_STR_LEN("\\f")); break;
                case '\n': strbufadd(dst, CONST_STR_LEN("\\n")); break;
                case '\r': strbufadd(dst, CONST_STR_LEN("\\r")); break;
                case '\t': strbufadd(dst, CONST_STR_LEN("\\t")); break;
                default: strbufadd(dst, &c, sizeof(char)); break;
            }
            i++;
        } else {
            char buf [8];
            size_t l = 0;
            int z = utf8_decode(&src[i], &l);
            snprintf(buf, sizeof buf, "\\u%04x", z);
            if (-1 == strbufadd(dst, buf, strlen(buf)))
                return -1;
            i += l;
        }
    }
    return 0;
}

void *mkstrset (size_t count, const char **strs) {
    void *ret = NULL, *p;
    size_t *lens = alloca(count * sizeof(size_t)), len = 0;
    for (size_t i = 0; i < count; ++i) {
        lens[i] = strlen(strs[i]);
        len += lens[i];
    }
    size_t size = 2 * sizeof(size_t) + sizeof(size_t) * count + len + len + sizeof(size_t) + 1;
    if (!(ret = p = malloc(size))) return NULL;
    memset(ret, 0, size);
    *((size_t*)p) = size;
    p += sizeof(size_t);
    *((size_t*)p) = count;
    p += sizeof(size_t);
    for (int i = 0; i < count; ++i) {
        size_t l = lens[i];
        *((size_t*)p) = l;
        p += sizeof(size_t);
        memcpy(p, strs[i], l+1);
        p += l + 1;
    }
    *((size_t*)p) = 0;
    p += sizeof(size_t);
    *((char*)p) = 0;
    return ret;
}

void *mkstrset2 (const char **strs, size_t count, size_t *idxs) {
    void *ret = NULL, *p;
    size_t *lens = alloca(count * sizeof(size_t)+sizeof(size_t)), len = 0;
    for (size_t i = 0; i < count; ++i) {
        lens[i] = strlen(strs[idxs[i]]);
        len += lens[i];
    }
    size_t size = 2 * sizeof(size_t) + sizeof(size_t) * count + len + len + sizeof(size_t) + 1;
    if (!(ret = p = malloc(size))) return NULL;
    memset(ret, 0, size);
    *((size_t*)p) = size;
    p += sizeof(size_t);
    *((size_t*)p) = count;
    p += sizeof(size_t);
    for (int i = 0; i < count; ++i) {
        size_t l = lens[i];
        *((size_t*)p) = l;
        p += sizeof(size_t);
        memcpy(p, strs[idxs[i]], l+1);
        p += l + 1;
    }
    *((size_t*)p) = 0;
    p += sizeof(size_t);
    *((char*)p) = 0;
    return ret;
}

void *strset_start (void *strset, size_t *len) {
    size_t l = *((size_t*)(strset + sizeof(size_t)));
    if (0 >= l)
        return NULL;
    if (len)
        *len = l;
    return strset + 2 * sizeof(size_t);
}

strptr_t strset_fetch (void **strset_ptr) {
    void *p = *strset_ptr;
    strptr_t ret = { .len = *((size_t*)p), .ptr = (char*)(p + sizeof(size_t)) };
    p += sizeof(size_t) + ret.len + 1;
    if (0 == *((size_t*)p) && 0 == *((char*)(p+sizeof(size_t))))
        *strset_ptr = NULL;
    else
        *strset_ptr = p;
    return ret;
}

strptr_t strset_get (void *strset, size_t index) {
    strptr_t ret = { .len = 0, .ptr = NULL };
    size_t size = *((size_t*)strset), count;
    strset += sizeof(size_t);
    count = *((size_t*)strset);
    strset += sizeof(size_t);
    if (index < count) {
        for (size_t i = 0; i < index; ++i) {
            size_t len = *((size_t*)strset);
            strset += sizeof(size_t) + len + 1;
        }
        ret.len = *((size_t*)strset);
        ret.ptr = (char*)(strset + sizeof(size_t));
    } else
        strset = strset + size - sizeof(size_t) - 1;
    return ret;
}

static char al_nums ['9'-'0'+1+'Z'-'A'+1+'z'-'a'+1];
static char als ['Z'-'A'+1+'z'-'a'+1];
static char al_reg_nums ['9'-'0'+1+'Z'-'A'+1];
static char al_regs ['Z'-'A'+1];

static void init () {
    char c,
         *p_al_nums = al_nums,
         *p_als = als,
         *p_al_reg_nums = al_reg_nums,
         *p_al_regs = al_regs;
    c = '0';
    while (c <= '9')
        *p_al_nums++ = *p_al_reg_nums++ = c++;
    c = 'A';
    while (c <= 'Z')
        *p_al_nums++ = *p_als++ = *p_al_reg_nums++ = *p_al_regs++ = c++;
    c = 'a';
    while (c <= 'z')
        *p_al_nums++ = *p_als++ = c++;
    srand(time(0));
}
static void init () __attribute__ ((constructor));

typedef int (*charconv_h) (int);

static void rand_gen (char *buf, size_t len, char *templ, size_t templ_len, charconv_h conv) {
    for (int i = 0; i < len; ++i)
        if (conv)
            buf[i] = conv(templ[rand() % templ_len]);
        else
            buf[i] = templ[rand() % templ_len];
}

#define RAND_WHAT_CHARS 0x000f
#define RAND_WHAT_REG 0x00f0
/**
 * strand - generate random string
 * @output: result string
 * @outlen: result string length
 * @type: generate parameter flags
 *
 * Returns 0 if success, -1 if invalid @type.
 * RAND_ALPHA - result contains only alpabetic characters
 * RAND_ALNUM - result contains alphabetic and numeric characters
 * RAND_UPPER - convert alpabetic characters to uppercase
 * RAND_LOWER - convert alpabetic characters to lowercase
 */
int strand (char *outbuf, size_t outlen, int type) {
    char *buf = NULL;
    size_t buf_len;
    charconv_h conv = NULL;
    int type_chars = type & RAND_WHAT_CHARS,
        type_reg = type & RAND_WHAT_REG;
    if ((type_chars & RAND_ALNUM))
        buf = al_nums;
    else
    if ((type_chars & RAND_ALPHA))
        buf = als;
    if (type_reg) {
        if (buf == al_nums)
            buf = al_reg_nums;
        else
        if (buf == als)
            buf = al_regs;
        if ((type_reg & RAND_UPPER))
            conv = toupper;
        else
        if ((type_reg & RAND_LOWER))
            conv = tolower;
    }
    if (buf == al_nums)
        buf_len = sizeof(al_nums);
    else
    if (buf == als)
        buf_len = sizeof(als);
    else
    if (buf == al_reg_nums)
        buf_len = sizeof(al_reg_nums);
    else
    if (buf == al_regs)
        buf_len = sizeof(al_regs);
    else
        return -1;
    rand_gen(outbuf, outlen, buf, buf_len, conv);
    outbuf[outlen] = '\0';
    return 0;
}
