#include "str.h"

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

str_t *mkstr (const char *str, size_t len, size_t chunk_size) {
    str_t *ret = stralloc(len, chunk_size);
    if (!ret) return NULL;
    if (0 == len) return NULL;
    ret->len = len;
    if (str && len) memcpy(ret->ptr, str, len);
    STR_ADD_NULL(ret);
    return ret;
}

wstr_t *wmkstr (const wchar_t *str, size_t len, size_t chunk_size) {
    wstr_t *ret = wstralloc(len, chunk_size);
    if (!ret) return NULL;
    if (0 == len) return NULL;
    ret->len = len;
    if (str && len) memcpy(ret->ptr, str, len * sizeof(wchar_t));
    WSTR_ADD_NULL(ret);
    return ret;
}

int strsize (str_t **str, size_t nlen, int flags) {
    str_t *s = *str;
    size_t bufsize = (nlen / s->chunk_size) * s->chunk_size + s->chunk_size;
    errno = 0;
    if (bufsize == s->bufsize) return 0;
    if (!(flags & STR_REDUCE) && bufsize < s->bufsize) return 0;
    s = realloc(s, sizeof(size_t)*3 + bufsize);
    if (!s) return -1;
    errno = ERANGE;
    s->bufsize = bufsize;
    *str = s;
    STR_ADD_NULL(*str);
    return 0;
}

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

int strput (str_t **str, const char *src, size_t src_len, int flags) {
    if (-1 == strsize(str, src_len, flags)) return -1;
    (*str)->len = src_len;
    memcpy((*str)->ptr, src, src_len);
    STR_ADD_NULL(*str);
    return 0;
}

str_t *strput2 (str_t *str, const char *src, size_t src_len, int flags) {
    if (-1 == strsize(&str, src_len, flags)) return NULL;
    str->len = src_len;
    memcpy(str->ptr, src, src_len);
    STR_ADD_NULL(str);
    return str;
}

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
    if (*str_len > 0) return 0;
    if (*str_len == 0) return 1;
    return -1;
}

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

char *strnchr (const char *s, int c, size_t str_len) {
    const char *e = s + str_len;
    while (s < e && *s != c) ++s;
    if (s >= e) return NULL;
    return (char*)s;
}

char *strnrchr (const char *s, int c, size_t str_len) {
    const char *e = s + str_len - 1;
    while (s <= e && *e != c) --e;
    if (e < s) return NULL;
    return (char*)e;
}

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

int cmpstr (const char *x, size_t x_len, const char *y, size_t y_len) {
    if (x_len < y_len) return -1;
    if (x_len > y_len) return 1;
    return strncmp(x, y, x_len);
}

int wcmpstr (const wchar_t *x, size_t x_len, const wchar_t *y, size_t y_len) {
    if (x_len < y_len) return -1;
    if (x_len > y_len) return 1;
    return wcsncmp(x, y, x_len);
}

int cmpcasestr (const char *x, size_t x_len, const char *y, size_t y_len) {
    if (x_len < y_len) return -1;
    if (x_len > y_len) return 1;
    return strncasecmp(x, y, x_len);
}

int wcmpcasestr (const wchar_t *x, size_t x_len, const wchar_t *y, size_t y_len) {
    if (x_len < y_len) return -1;
    if (x_len > y_len) return 1;
    return wcsncasecmp(x, y, x_len);
}

int strbufalloc (strbuf_t *strbuf, size_t len, size_t chunk_size) {
    size_t bufsize = (len / chunk_size) * chunk_size + chunk_size;
    if (len == bufsize) bufsize += chunk_size;
    char *buf = calloc(1, bufsize);
    if (!buf) return -1;
    buf[0] = '\0';
    strbuf->ptr = buf;
    strbuf->ptr[0] = '\0';
    strbuf->len = 0;
    strbuf->bufsize = bufsize;
    strbuf->chunk_size = chunk_size;
    return 0;
}

int strbufsize (strbuf_t *strbuf, size_t nlen, int flags) {
    char *buf = strbuf->ptr;
    size_t bufsize = (nlen / strbuf->chunk_size) * strbuf->chunk_size + strbuf->chunk_size;
    errno = 0;
    if (bufsize == strbuf->bufsize) return 0;
    if (!(flags & STR_REDUCE) && bufsize < strbuf->bufsize) return 0;
    buf = realloc(buf, bufsize);
    if (!buf) return -1;
    errno = ERANGE;
    strbuf->bufsize = bufsize;
    strbuf->ptr = buf;
    if (nlen < strbuf->len) strbuf->len = nlen;
    return 0;
}

int strbufput (strbuf_t *strbuf, const char *src, size_t src_len, int flags) {
    if (-1 == strbufsize(strbuf, src_len, flags)) return -1;
    strbuf->len = src_len;
    memcpy(strbuf->ptr, src, src_len);
    return 0;
}

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

int strbufset (strbuf_t *buf, const char c, size_t len) {
    if (-1 == strbufsize(buf, len, 0))
        return -1;
    buf->len = len;
    memset(buf->ptr, c, len);
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

ssize_t base64_encode(const unsigned char *data, size_t input_length, char *encoded_data, size_t output_length) {
    ssize_t i = 0, j = 0;
    for (i = 0, j = 0; i < input_length;) {
        uint32_t octet_a = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_b = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t octet_c = i < input_length ? (unsigned char)data[i++] : 0;
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    j--;
    for (int i = 0; i < mod_table[input_length % 3]; i++)
        //encoded_data[output_length - 1 - i] = '=';
        encoded_data[j++] = '=';
    return j;
}

str_t *str_base64_encode (const char *buf, size_t bufsize, size_t chunk_size) {
    size_t len = ENCODED_LEN(bufsize);
    str_t *res = stralloc(ENCODED_LEN(bufsize), chunk_size);
    memset(res->ptr, 0, res->bufsize);
    base64_encode((const unsigned char*)buf, bufsize, res->ptr, len);
    res->len = len;
    return res;
}

int strbuf_base64_encode (strbuf_t *encoded, const char *input, size_t input_len, size_t chunk_size) {
    size_t len = ENCODED_LEN(input_len);
    if (-1 == strbufalloc(encoded, len, chunk_size)) return -1;
    memset(encoded->ptr, 0, encoded->bufsize);
    base64_encode((const unsigned char*)input, input_len, encoded->ptr, len);
    encoded->len = len;
    return 0;
}

#define DECODED_LEN(X) X / 4 * 3;

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

inline void strwupper (char *str, size_t str_len, locale_t locale) {
    str_ul_conv(str, str_len, locale, towupper_l);
}

inline void strwlower (char *str, size_t str_len, locale_t locale) {
    str_ul_conv(str, str_len, locale, towlower_l);
}

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
int strand (char *outbuf, size_t outlen, int flags) {
    char *buf = NULL;
    size_t buf_len;
    charconv_h conv = NULL;
    int type_chars = flags & RAND_WHAT_CHARS,
        type_reg = flags & RAND_WHAT_REG;
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

#define GET_UINT32(n,b,i)                       \
{                                               \
    (n) = ( (uint32_t) (b)[(i)    ] << 24 )       \
        | ( (uint32_t) (b)[(i) + 1] << 16 )       \
        | ( (uint32_t) (b)[(i) + 2] <<  8 )       \
        | ( (uint32_t) (b)[(i) + 3]       );      \
}

#define PUT_UINT32(n,b,i)                       \
{                                               \
    (b)[(i)    ] = (uint8_t) ( (n) >> 24 );       \
    (b)[(i) + 1] = (uint8_t) ( (n) >> 16 );       \
    (b)[(i) + 2] = (uint8_t) ( (n) >>  8 );       \
    (b)[(i) + 3] = (uint8_t) ( (n)       );       \
}

void sha1_init (sha1_t *ctx)
{
    ctx->total[0] = 0;
    ctx->total[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xEFCDAB89;
    ctx->state[2] = 0x98BADCFE;
    ctx->state[3] = 0x10325476;
    ctx->state[4] = 0xC3D2E1F0;
}

static void sha1_process (sha1_t *ctx, uint8_t data[64])
{
    uint32_t temp, A, B, C, D, E, W[16];

    GET_UINT32( W[0],  data,  0 );
    GET_UINT32( W[1],  data,  4 );
    GET_UINT32( W[2],  data,  8 );
    GET_UINT32( W[3],  data, 12 );
    GET_UINT32( W[4],  data, 16 );
    GET_UINT32( W[5],  data, 20 );
    GET_UINT32( W[6],  data, 24 );
    GET_UINT32( W[7],  data, 28 );
    GET_UINT32( W[8],  data, 32 );
    GET_UINT32( W[9],  data, 36 );
    GET_UINT32( W[10], data, 40 );
    GET_UINT32( W[11], data, 44 );
    GET_UINT32( W[12], data, 48 );
    GET_UINT32( W[13], data, 52 );
    GET_UINT32( W[14], data, 56 );
    GET_UINT32( W[15], data, 60 );

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define R(t)                                            \
(                                                       \
    temp = W[(t -  3) & 0x0F] ^ W[(t - 8) & 0x0F] ^     \
           W[(t - 14) & 0x0F] ^ W[ t      & 0x0F],      \
    ( W[t & 0x0F] = S(temp,1) )                         \
)

#define P(a,b,c,d,e,x)                                  \
{                                                       \
    e += S(a,5) + F(b,c,d) + K + x; b = S(b,30);        \
}

    A = ctx->state[0];
    B = ctx->state[1];
    C = ctx->state[2];
    D = ctx->state[3];
    E = ctx->state[4];

#define F(x,y,z) (z ^ (x & (y ^ z)))
#define K 0x5A827999

    P( A, B, C, D, E, W[0]  );
    P( E, A, B, C, D, W[1]  );
    P( D, E, A, B, C, W[2]  );
    P( C, D, E, A, B, W[3]  );
    P( B, C, D, E, A, W[4]  );
    P( A, B, C, D, E, W[5]  );
    P( E, A, B, C, D, W[6]  );
    P( D, E, A, B, C, W[7]  );
    P( C, D, E, A, B, W[8]  );
    P( B, C, D, E, A, W[9]  );
    P( A, B, C, D, E, W[10] );
    P( E, A, B, C, D, W[11] );
    P( D, E, A, B, C, W[12] );
    P( C, D, E, A, B, W[13] );
    P( B, C, D, E, A, W[14] );
    P( A, B, C, D, E, W[15] );
    P( E, A, B, C, D, R(16) );
    P( D, E, A, B, C, R(17) );
    P( C, D, E, A, B, R(18) );
    P( B, C, D, E, A, R(19) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0x6ED9EBA1

    P( A, B, C, D, E, R(20) );
    P( E, A, B, C, D, R(21) );
    P( D, E, A, B, C, R(22) );
    P( C, D, E, A, B, R(23) );
    P( B, C, D, E, A, R(24) );
    P( A, B, C, D, E, R(25) );
    P( E, A, B, C, D, R(26) );
    P( D, E, A, B, C, R(27) );
    P( C, D, E, A, B, R(28) );
    P( B, C, D, E, A, R(29) );
    P( A, B, C, D, E, R(30) );
    P( E, A, B, C, D, R(31) );
    P( D, E, A, B, C, R(32) );
    P( C, D, E, A, B, R(33) );
    P( B, C, D, E, A, R(34) );
    P( A, B, C, D, E, R(35) );
    P( E, A, B, C, D, R(36) );
    P( D, E, A, B, C, R(37) );
    P( C, D, E, A, B, R(38) );
    P( B, C, D, E, A, R(39) );

#undef K
#undef F

#define F(x,y,z) ((x & y) | (z & (x | y)))
#define K 0x8F1BBCDC

    P( A, B, C, D, E, R(40) );
    P( E, A, B, C, D, R(41) );
    P( D, E, A, B, C, R(42) );
    P( C, D, E, A, B, R(43) );
    P( B, C, D, E, A, R(44) );
    P( A, B, C, D, E, R(45) );
    P( E, A, B, C, D, R(46) );
    P( D, E, A, B, C, R(47) );
    P( C, D, E, A, B, R(48) );
    P( B, C, D, E, A, R(49) );
    P( A, B, C, D, E, R(50) );
    P( E, A, B, C, D, R(51) );
    P( D, E, A, B, C, R(52) );
    P( C, D, E, A, B, R(53) );
    P( B, C, D, E, A, R(54) );
    P( A, B, C, D, E, R(55) );
    P( E, A, B, C, D, R(56) );
    P( D, E, A, B, C, R(57) );
    P( C, D, E, A, B, R(58) );
    P( B, C, D, E, A, R(59) );

#undef K
#undef F

#define F(x,y,z) (x ^ y ^ z)
#define K 0xCA62C1D6

    P( A, B, C, D, E, R(60) );
    P( E, A, B, C, D, R(61) );
    P( D, E, A, B, C, R(62) );
    P( C, D, E, A, B, R(63) );
    P( B, C, D, E, A, R(64) );
    P( A, B, C, D, E, R(65) );
    P( E, A, B, C, D, R(66) );
    P( D, E, A, B, C, R(67) );
    P( C, D, E, A, B, R(68) );
    P( B, C, D, E, A, R(69) );
    P( A, B, C, D, E, R(70) );
    P( E, A, B, C, D, R(71) );
    P( D, E, A, B, C, R(72) );
    P( C, D, E, A, B, R(73) );
    P( B, C, D, E, A, R(74) );
    P( A, B, C, D, E, R(75) );
    P( E, A, B, C, D, R(76) );
    P( D, E, A, B, C, R(77) );
    P( C, D, E, A, B, R(78) );
    P( B, C, D, E, A, R(79) );

#undef K
#undef F

    ctx->state[0] += A;
    ctx->state[1] += B;
    ctx->state[2] += C;
    ctx->state[3] += D;
    ctx->state[4] += E;
}

void sha1_update (sha1_t *ctx, uint8_t *input, uint32_t length)
{
    uint32_t left, fill;

    if( ! length ) return;

    left = ( ctx->total[0] >> 3 ) & 0x3F;
    fill = 64 - left;

    ctx->total[0] += length <<  3;
    ctx->total[1] += length >> 29;

    ctx->total[0] &= 0xFFFFFFFF;
    ctx->total[1] += ctx->total[0] < ( length << 3 );

    if( left && length >= fill )
    {
        memcpy( (void *) (ctx->buffer + left), (void *) input, fill );
        sha1_process( ctx, ctx->buffer );
        length -= fill;
        input  += fill;
        left = 0;
    }

    while( length >= 64 )
    {
        sha1_process( ctx, input );
        length -= 64;
        input  += 64;
    }

    if( length )
    {
        memcpy( (void *) (ctx->buffer + left), (void *) input, length );
    }
}

static uint8_t sha1_padding[64] =
{
 0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

void sha1_done (sha1_t *ctx, uint8_t digest[20])
{
    uint32_t last, padn;
    uint8_t msglen[8];

    PUT_UINT32( ctx->total[1], msglen, 0 );
    PUT_UINT32( ctx->total[0], msglen, 4 );

    last = ( ctx->total[0] >> 3 ) & 0x3F;
    padn = ( last < 56 ) ? ( 56 - last ) : ( 120 - last );

    sha1_update( ctx, sha1_padding, padn );
    sha1_update( ctx, msglen, 8 );

    PUT_UINT32( ctx->state[0], digest,  0 );
    PUT_UINT32( ctx->state[1], digest,  4 );
    PUT_UINT32( ctx->state[2], digest,  8 );
    PUT_UINT32( ctx->state[3], digest, 12 );
    PUT_UINT32( ctx->state[4], digest, 16 );
}

