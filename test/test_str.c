#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include "../include/libex/str.h"
#include "../include/libex/file.h"
#include "../include/libex/msg.h"

#define RULE "dbname=world"

#define STR_DELIMS STR_SPACES

void print_tok (strptr_t *tok, str_t **ret) {
    (*ret)->len = 0;
    strnadd(ret, tok->ptr, tok->len);
    (*ret)->ptr[(*ret)->len] = '\0';
    printf("%s\n", (*ret)->ptr);
}

void test_strntok () {
    strptr_t tok;
    char *rule=RULE;
    size_t rule_len = strlen(RULE);
    strntok(&rule, &rule_len, CONST_STR_LEN("=;"), &tok);
    strntok(&rule, &rule_len, CONST_STR_LEN("=;"), &tok);
}

void test_strepl () {
    str_t *str = stralloc(8, 8);
    strnadd(&str, CONST_STR_LEN("aaaaaaaa"));
    strepl(&str, str->ptr+3, 3, CONST_STR_LEN("bbbbbb"));
    str->ptr[str->len] = '\0';
    STR_ADD_NULL(str);
    printf("%s\n", str->ptr);
    free(str);
    str = mkstr(CONST_STR_LEN("ssss ffff :x"), 16);
    char *p = strnchr(str->ptr, ':', str->len);
    strepl(&str, p, 2, CONST_STR_LEN("?"));
    STR_ADD_NULL(str);
    printf("%s\n", str->ptr);
}

void test_replall () {
    str_t *str = mkstr(CONST_STR_LEN("asd \"fdhu\" \" gfd\" 65\"432\" ggfds\"\""), 8);
    char *p = str->ptr;
    size_t l = str->len;
    printf("%s\n", str->ptr);
    while ((p = strnchr(p, '"', l))) {
        size_t ll = (uintptr_t)p - (uintptr_t)str->ptr;
        strepl(&str, p, 1, CONST_STR_LEN("\\\""));
        p = str->ptr + ll + 2;
        l = str->len - ((uintptr_t)p - (uintptr_t)str->ptr);
    }
    printf("%s\n", str->ptr);
    free(str);
}


static str_t *hdrepl (str_t *str) {
    char *p = str->ptr;
    size_t l = str->len;
    while ((p = strnchr(p, '"', l))) {
        size_t ll = (uintptr_t)p - (uintptr_t)str->ptr;
        strepl(&str, p, 1, CONST_STR_LEN("\\\""));
        p = str->ptr + ll + 2;
        l = str->len - ((uintptr_t)p - (uintptr_t)str->ptr);
    }
    return str;
}

void test_repl2 () {
    str_t *str = load_all_file("./test.txt", 64, 100000);
    if (str) {
        str = hdrepl(str);
        printf("%s\n", str->ptr);
        free(str);
    }
}

void str_size () {
    str_t *str = stralloc(0, 32);
    strsize(&str, 6, 0);
#ifdef __x86_64__
    printf("%lu\n", str->bufsize);
    strsize(&str, 30, 0);
    printf("%lu\n", str->bufsize);
    strsize(&str, 32, 0);
    printf("%lu\n", str->bufsize);
    strsize(&str, 40, 0);
    printf("%lu\n", str->bufsize);
    strsize(&str, 10, STR_REDUCE);
    printf("%lu\n", str->bufsize);
    strsize(&str, 10, 0);
    printf("%lu\n", str->bufsize);
#else
    printf("%u\n", str->bufsize);
    strsize(&str, 30, 0);
    printf("%u\n", str->bufsize);
    strsize(&str, 32, 0);
    printf("%u\n", str->bufsize);
    strsize(&str, 40, 0);
    printf("%u\n", str->bufsize);
    strsize(&str, 10, STR_REDUCE);
    printf("%u\n", str->bufsize);
    strsize(&str, 10, 0);
    printf("%u\n", str->bufsize);
#endif
    free(str);
}

void str_del () {
    str_t *str = mkstr(CONST_STR_LEN("qwertyuiopasdfghjklzxcvbnm"), 32);
    str->ptr[str->len] = '\0';
    printf("%s\n", str->ptr);
    strdel(&str, str->ptr + str->len - 3, 5, 0);
    str->ptr[str->len] = '\0';
    printf("%s\n", str->ptr);
}

void str_inc () {
    str_t *str = stralloc(8, 8);
    for (int i = 0; i < 64; ++i)
        strnadd(&str, CONST_STR_LEN("aaaaaaaaaa"));
    free(str);
    strbuf_t buf;
    strbufalloc(&buf, 8, 8);
    for (int i = 0; i < 64; ++i)
        strbufadd(&buf, CONST_STR_LEN("1234567"));
    free(buf.ptr);
}

void str_nstr () {
    char *str = "aaaaaaaabb";
    size_t len = sizeof("aaaaaaaabb")-1;
    char *p = strnstr(str, len, CONST_STR_LEN("bb"));
    if (p) printf("%s\n", p);
}

void str_str () {
    str_t *str = mkstr(CONST_STR_LEN("123"), 8);
    STR_ADD_NULL(str);
    free(str);
    str = mkstr(CONST_STR_LEN(""), 8);
    STR_ADD_NULL(str);
    free(str);
    str = mkstr(CONST_STR_LEN("12345678"), 8);
    STR_ADD_NULL(str);
    free(str);
}

void test_strnadd () {
    str_t *x = mkstr(CONST_STR_LEN("a"), 8);
    for (int i = 0; i < 100; ++i) {
        strnadd(&x, CONST_STR_LEN("a"));
        STR_ADD_NULL(x);
        printf("%s\n", x->ptr);
    }
    free(x);
}

void test_pad () {
    str_t *str = mkstr(CONST_STR_LEN("asdfghjk"), 16);
    strpad(&str, 15, ' ', STR_LEFT | STR_MAYBE_UTF);
    STR_ADD_NULL(str);
    printf("<%s>\n", str->ptr);
    free(str);
    str = mkstr(CONST_STR_LEN("фывапрол"), 16);
    strpad(&str, 15, ' ', STR_LEFT | STR_MAYBE_UTF);
    STR_ADD_NULL(str);
    printf("<%s>\n", str->ptr);
    free(str);
    
    str = mkstr(CONST_STR_LEN("asdfghjk"), 16);
    strpad(&str, 15, ' ', STR_CENTER | STR_MAYBE_UTF);
    STR_ADD_NULL(str);
    printf("<%s>\n", str->ptr);
    free(str);
    str = mkstr(CONST_STR_LEN("фывапрол"), 16);
    strpad(&str, 15, ' ', STR_CENTER | STR_MAYBE_UTF);
    STR_ADD_NULL(str);
    printf("<%s>\n", str->ptr);
    free(str);

    str = mkstr(CONST_STR_LEN("asdfghjk"), 16);
    strpad(&str, 15, ' ', STR_RIGHT | STR_MAYBE_UTF);
    STR_ADD_NULL(str);
    printf("<%s>\n", str->ptr);
    free(str);
    str = mkstr(CONST_STR_LEN("фывапрол"), 16);
    strpad(&str, 15, ' ', STR_RIGHT | STR_MAYBE_UTF);
    STR_ADD_NULL(str);
    printf("<%s>\n", str->ptr);
    free(str);
}

void test_wstrlen () {
#ifdef __x86_64__
    printf("%lu\n", strwlen(CONST_STR_LEN("asdfghjk")));
    printf("%lu\n", strwlen(CONST_STR_LEN("фывапрол")));
#else
    printf("%u\n", strwlen(CONST_STR_LEN("asdfghjk")));
    printf("%u\n", strwlen(CONST_STR_LEN("фывапрол")));
#endif
}

void test_strbuf () {
    strbuf_t str;
    strbufalloc(&str, 8, 8);
    strbufput(&str, CONST_STR_LEN("good luck"), 0);
    STR_ADD_NULL(&str);
    printf("%s\n", str.ptr);
    strbufadd(&str, CONST_STR_LEN(", my friend"));
    STR_ADD_NULL(&str);
    printf("%s\n", str.ptr);
    free(str.ptr);
}

void test_unescape () {
    strbuf_t buf;
    strbufalloc(&buf,128, 8);
    strbuf_escape(&buf, CONST_STR_LEN("Денег нет, но вы держитесь, и хорошего вам настроения"));
    printf("%s\n", buf.ptr);
    str_t *str = str_unescape(buf.ptr, buf.len, 1024);
    printf("%s\n", str->ptr);
    free(buf.ptr);
    free(str);
}

char base64_digit (n) unsigned n; {
  if (n < 10) return n - '0';
  else if (n < 10 + 26) return n - 'a';
  else if (n < 10 + 26 + 26) return n - 'A';
  else assert(0);
  return 0;
}

unsigned char base64_decode_digit(char c) {
  switch (c) {
    case '=' : return 62;
    case '.' : return 63;
    default  :
      if (isdigit(c)) return c - '0';
      else if (islower(c)) return c - 'a' + 10;
      else if (isupper(c)) return c - 'A' + 10 + 26;
      else assert(0);
  }
  return 0xff;
}

unsigned base64_decode(char *s, size_t len) {
  char *p;
  unsigned n = 0;

/*  for (p = s; *p; p++)
    n = 64 * n + base64_decode_digit(*p);*/
  for (size_t i = 0; i < len; ++i) {
    n = 64 * n + base64_decode_digit(*p);
    ++p;
  }

  return n;
}

const char *encoded64 = "PXRoq2pJLbIZZnI8Enjo/Eivje5dHXJSBqPy4QaqvdlqqU1XwztgI+7waLnq/z9CQA6DxXJBlrj+\nOONlEKQS60II7bPLggEk15Xt+PTkcwaI30f2Ooqj1eIrPk/Rq29UrOFxtcPQygyFfJRr6fuIugL6\nrt6jqcp+osTQEDD1QEY=\n";

void test_base64 () {
    str_t *str1 = str_base64_encode(CONST_STR_LEN("Good luck!"), 8);
    if (str1) {
        printf("%s\n", str1->ptr);
        str_t *str2 = str_base64_decode(str1->ptr, str1->len, 8);
        if (str2) {
            printf("%s\n", str2->ptr);
            printf("%s\n", 0 == cmpstr(str2->ptr, str2->len, CONST_STR_LEN("Good luck!")) ? "Yes" : "No");
            free(str2);
        }
        free(str1);
    }
    strbuf_t buf1, buf2;
    if (0 == strbuf_base64_encode(&buf1, CONST_STR_LEN("Good luck!"), 8)) {
        printf("%s\n", buf1.ptr);
        if (0 == strbuf_base64_decode(&buf2, buf1.ptr, buf1.len, 8)) {
            printf("%s\n", buf2.ptr);
            printf("%s\n", 0 == cmpstr(buf2.ptr, buf2.len, CONST_STR_LEN("Good luck!")) ? "Yes" : "No");
            free(buf2.ptr);
        }
        free(buf1.ptr);
    }
}

void test_str_close () {
    str_t *src = mkstr(CONST_STR_LEN("good luck!"), 16),
          *dst = strclone(src);
    printf("src: [\"%s\", " SIZE_FMT ", " SIZE_FMT "]\n", src->ptr, src->len, src->bufsize);
    printf("dst: [\"%s\", " SIZE_FMT ", " SIZE_FMT "]\n", dst->ptr, dst->len, dst->bufsize);
    free(dst);
    free(src);
}

void test_strset () {
    strptr_t s;
    void *strset, *strset_ptr;
    const char *strs [] = { "111111", "222222222222", "33333", "4444444" };
    strset = mkstrset(4, strs);
    strset_ptr = strset_start(strset, NULL);
    while (strset_ptr) {
        s = strset_fetch(&strset_ptr);
        printf("%s\n", s.ptr);
    }
    s = strset_get(strset, 1);
    printf("%s\n", s.ptr);
    s = strset_get(strset, 2);
    printf("%s\n", s.ptr);
    s = strset_get(strset, 10);
    free(strset);
    printf("\n");
    const char *strs2 [] = {"111","222","333","444","555","666","777","888","999","000"};
    size_t idxs [4];
    idxs[0] = 1; idxs[1] = 3; idxs[2] = 5; idxs[3] = 7;
    strset = mkstrset2(strs2, 4, idxs);
    strset_ptr = strset_start(strset, NULL);
    while (strset_ptr) {
        s = strset_fetch(&strset_ptr);
        printf("%s\n", s.ptr);
    }
    free(strset);
}

extern char _binary___test_test_urlenc_start;
extern char _binary___test_test_urlenc_end;
extern char _binary___test_test_urldec_start;
extern char _binary___test_test_urldec_end;

void test_url () {
    char *p = &_binary___test_test_urlenc_start, *e = &_binary___test_test_urlenc_end;
    size_t l = (uintptr_t)e - (uintptr_t)p;
    str_t *str = str_url_encode(p, l, 8);
    printf("%s\n", str->ptr);
    free(str);
    p = &_binary___test_test_urldec_start, e = &_binary___test_test_urldec_end;
    l = (uintptr_t)e - (uintptr_t)p;
    str = str_url_decode(p, l, 8);
    printf("%s\n", str->ptr);
    free(str);
}

void test_rev () {
    char *s = "123456789";
    size_t l = strlen(s);
    str_t *str = strev(s, l, 8);
    printf("%s\n", str->ptr);
    free(str);
}

void test_rand () {
    char buf [16];
    strand(buf, sizeof(buf), RAND_ALPHA);
    printf("%s\n", buf);
    strand(buf, sizeof(buf), RAND_ALNUM);
    printf("%s\n", buf);
    strand(buf, sizeof(buf), RAND_ALPHA | RAND_UPPER);
    printf("%s\n", buf);
    strand(buf, sizeof(buf), RAND_ALPHA | RAND_LOWER);
    printf("%s\n", buf);
    strand(buf, sizeof(buf), RAND_ALNUM | RAND_UPPER);
    printf("%s\n", buf);
    strand(buf, sizeof(buf), RAND_ALNUM | RAND_LOWER);
    printf("%s\n", buf);
}

// utf-8
#define MM_MSG1 "Денег нет, но вы держитесь"
#define MM_MSG1_LEN sizeof(MM_MSG1)-1
#define MM_MSG2 "И ХОРОШЕГО ВАМ НАСТРОЕНИЯ"
#define MM_MSG2_LEN sizeof(MM_MSG2)-1
void test_strw_c () {
    char *mm_msg1 = strdup(MM_MSG1), *mm_msg2 = strdup(MM_MSG2);
    locale_t loc = newlocale(LC_ALL_MASK, "", (locale_t)0);
    strwupper(mm_msg1, MM_MSG1_LEN, loc);
    printf("%s, ", mm_msg1);
    strwlower(mm_msg2, MM_MSG2_LEN, loc);
    printf("%s\n", mm_msg2);
    freelocale(loc);
    free(mm_msg1);
    free(mm_msg2);
}

#define SRC_STR "1234567890qwertyuiopasdfghjklzxcvbnm"
void test_hexstr () {
    //str_t *str = strhex(CONST_STR_LEN("mint(address,uint256)"), 8),
    str_t *str = strhex("", CONST_STR_LEN(SRC_STR), 8),
          *str2 = hexstr(str->ptr, str->len, 8);
    printf("%s\n", str->ptr);
    free(str);
    printf("%s\n", str2->ptr);
    if (0 == strcmp(str2->ptr, SRC_STR))
        printf("YES\n");
    free(str2);
}

void test_concat () {
    str_t *str = strconcat(8, CONST_STR_LEN("123"), CONST_STR_LEN("456"), CONST_STR_LEN("789"), NULL);
    printf("%s\n", str->ptr);
    free(str);
}

#define MSG_TEST 2
strptr_t tl [] = {
    { .ptr = "123", .len = 3 },
    { .ptr = "1234567", .len = 7 },
    { .ptr = "123456789", .len = 9 }
};
int on_msg_item (msgbuf_t *msg, void *data, void *userdata) {
    strptr_t *s = (strptr_t*)data;
    if (0 == msg_setstr(msg, s->ptr, s->len))
        return MSG_INSERTED;
    return MSG_NOT_INSERTED;
}
int on_get_msg (msgbuf_t *msg, void *dummy, void *userdata) {
    strptr_t s;
    if (-1 != msg_getstr(msg, &s))
        printf(" %s\n", s.ptr);
    return ENUM_CONTINUE;
}
void test_msg () {
    list_t *lst = lst_alloc(NULL);
    msgbuf_t msg;
    void *buf;
    uint32_t len;
    for (int i = 0; i < 3; ++i)
        lst_adde(lst, &tl[i]);
    msg_create_request(&msg, MSG_TEST, CONST_STR_LEN("qwerty"), 32, 32);
    msg_seti32(&msg, 48);
    msg_setstr(&msg, CONST_STR_LEN("good luck!"));
    msg_setlist(&msg, lst, on_msg_item, NULL);
    
    buf = msg.ptr;
    len = msg.len;

    msg_load_request(&msg, buf, len);
    
    int32_t i;
    strptr_t s;
    printf("method: %u\n", msg.method);
    printf("cookie %s\n", msg.cookie.ptr);
    if (-1 != msg_geti32(&msg, &i))
        printf("param1: %i\n", i);
    if (-1 != msg_getstr(&msg, &s))
        printf("param2: %s\n", s.ptr);
    printf("items:\n");
    msg_enum(&msg, on_get_msg, NULL);
    msg_clear(&msg);
    lst_free(lst);
}

int main () {
/*    test_strntok();
    test_strepl();
    str_size();
    str_del();
    str_nstr();
    str_str();
    test_strnadd();
    str_inc();
    test_pad();
    test_wstrlen();
    test_strbuf();
    test_unescape();
    test_url();
    test_base64();
    test_str_close();
    test_strset();
    test_rev();
    test_repl2();
    test_rand();
    test_strw_c();
    test_hexstr();
    test_concat();*/
    test_msg();
    return 0;
}
