#include "../include/libex/file.h"
#include <errno.h>

void test_log () {
    loginit("./log");
    slogf("%s %d", "a", 0);
    slogf("%s %d", "b", 1);
    slogf("%s %d", "c", 2);
}

void test_conf () {
//! [load conf]
    str_t *string_val = NULL;
    int integer_val = -1;
    double double_val = -1.0;
    load_conf("test.conf", CONF_HANDLER
        ASSIGN_CONF_STR(string_val, "string")
        ASSIGN_CONF_INT(integer_val, "integer")
        ASSIGN_CONF_DOUBLE(double_val, "double")
    CONF_HANDLER_END);
//! [load conf]
    if (string_val) {
        printf("string: %s\n", string_val->ptr);
        free(string_val);
    }
    printf("integer: %d\n", integer_val);
    printf("double: %f\n", double_val);
}
#if 0
void enum_ini (void *a, void *userdata) {
    ini_item_t *x = (ini_item_t*)a;
    STR_ADD_NULL(x->group);
    printf("[%s]\n", x->group->ptr);
    lenum(x->items, enum_conf, NULL);
}

void test_ini () {
    ini_t *ini = loadini("./test.ini");
    if (ini) {
        lenum(ini, enum_ini, NULL);
        freeini(ini);
    }
}
#endif
static int on_print_fname (const char *path, void *dummy, int flags, struct stat *st) {
    if (S_ISDIR(st->st_mode)) {
        if ((flags & S_INDIR))
            printf("->");
        else
            printf("<-");
    }
    printf("%s\n", path);
    return ENUM_CONTINUE;
}

void test_ls () {
    ls(".", on_print_fname, NULL, 0, 0);
}

void test_getfname () {
    strptr_t s;
    if (0 == getfname(CONST_STR_LEN("proj/libex2/include/libex/file.h"), &s))
        printf("%s\n", s.ptr);
    if (0 == getfname(CONST_STR_LEN("file.h"), &s))
        printf("%s\n", s.ptr);
    if (0 == getfname(CONST_STR_LEN("/file.h"), &s))
        printf("%s\n", s.ptr);
    if (0 == getoname(CONST_STR_LEN("proj/libex2/include/libex/file.h"), &s)) {
        char *str = strndup(s.ptr, s.len);
        printf("%s\n", str);
        free(str);
    }
    if (0 == getoname(CONST_STR_LEN("file.h"), &s)) {
        char *str = strndup(s.ptr, s.len);
        printf("%s\n", str);
        free(str);
    }
    if (0 == getoname(CONST_STR_LEN("/file.h"), &s)) {
        char *str = strndup(s.ptr, s.len);
        printf("%s\n", str);
        free(str);
    }
}

void test_path_combine () {
    str_t *path = path_combine("/usr", "local", "include", NULL);
    printf("%s\n", path->ptr);
    free(path);
    path = path_combine("/usr", "/local", "include", NULL);
    printf("%s\n", path->ptr);
    free(path);
    path = path_combine("/usr/", "/local", "/include", NULL);
    printf("%s\n", path->ptr);
    path = path_add_path(path, "libex", NULL);
    printf("%s\n", path->ptr);
    free(path);
}

void test_path_split () {
    str_t *dir, *fname, *suf;
    path_split(CONST_STR_LEN("/home/user/proj/file.ext"), &dir, &fname, &suf);
    printf("%s\n%s\n%s\n\n", dir->ptr, fname->ptr, suf->ptr);
    free(dir); free(fname); free(suf);
    path_split(CONST_STR_LEN("/home/user/proj/file"), &dir, &fname, &suf);
    printf("%s\n%s\n\n", dir->ptr, fname->ptr);
    free(dir); free(fname); free(suf);
    path_split(CONST_STR_LEN("file.ext"), &dir, &fname, &suf);
    printf("%s\n%s\n\n", fname->ptr, suf->ptr);
    free(fname); free(suf);
    path_split(CONST_STR_LEN("file"), &dir, &fname, &suf);
    printf("%s\n", fname->ptr);
    free(fname); free(suf);
}

void test_spec_path () {
    str_t *path;
    if ((path = get_spec_path(DIR_CURRENT))) {
        printf("%s\n", path->ptr);
        free(path);
    }
}

void test_expand_path () {
    str_t *path;
    path = path_expand(CONST_STR_LEN("/dir1/dir2/dir3"), CONST_STR_LEN("../file"));
    if (path) { printf("%s\n", path->ptr); free(path); }
    path = path_expand(CONST_STR_LEN("/dir1/dir2/dir3"), CONST_STR_LEN("../../file"));
    if (path) { printf("%s\n", path->ptr); free(path); }
    path = path_expand(CONST_STR_LEN("/dir1/dir2/dir3"), CONST_STR_LEN("../../../file"));
    if (path) { printf("%s\n", path->ptr); free(path); }
    path = path_expand(CONST_STR_LEN("/dir1/dir2/dir3"), CONST_STR_LEN("../../../../file"));
    if (path) { printf("%s\n", path->ptr); free(path); }
    path = path_expand(CONST_STR_LEN("/dir1/dir2/dir3"), CONST_STR_LEN(".././file"));
    if (path) { printf("%s\n", path->ptr); free(path); }
    path = path_expand(CONST_STR_LEN("/dir1/dir2/dir3"), CONST_STR_LEN("./../file"));
    if (path) { printf("%s\n", path->ptr); free(path); }
    path = path_expand(CONST_STR_LEN("/dir1/dir2/dir3"), CONST_STR_LEN("file"));
    if (path) { printf("%s\n", path->ptr); free(path); }
}

void test_getdir () {
    strptr_t dir;
    getdir(CONST_STR_LEN("/dir1/dir2/file"), &dir);
    if (dir.len > 0) {
        char *p = strndup(dir.ptr, dir.len);
        printf("%s\n", p);
        free(p);
    }
}

void test_tempfile () {
    str_t *str = mktempnam(CONST_STR_LEN("/dir/dir1/"), CONST_STR_LEN("file.ext"));
    printf("%s\n", str->ptr);
    free(str);
    str = mktempnam(NULL, 0, CONST_STR_LEN("file.ext"));
    printf("%s\n", str->ptr);
    free(str);
}

void test_load_all_file () {
    pid_t pid = getpid();
    char buf [16];
    snprintf(buf, sizeof(buf), "%d", pid);
    str_t *path = path_combine("/proc", buf, "cmdline");
    if (path) {
        str_t *cmdline = load_all_file(path->ptr, 8, 1024);
        for (int i = 0; i < cmdline->len; ++i)
            if (cmdline->ptr[i] == '\0') cmdline->ptr[i] = ' ';
        printf("%s\n", cmdline->ptr);
        free(cmdline);
        free(path);
    }
}

void test_copy_file (int argc, const char *argv[]) {
    if (argc < 3)
        return;
    copy_file(argv[1], argv[2]);
}

int main (int argc, const char *argv[]) {
/*    test_log();
    test_path_combine();
    test_conf();
    test_spec_path();
    test_expand_path();
    test_getdir();
    test_getfname();
    test_tempfile();
    test_load_all_file();
    test_copy_file(argc, argv);
    test_ls();*/
    test_load_all_file();
    return 0;
}
