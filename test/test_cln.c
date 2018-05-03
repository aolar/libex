#include "net.h"

int thread_count = 0, task_count = 0;

static void inout (int fd) {
    int n = rand();
    strbuf_t buf;
    strbufalloc(&buf, 64, 64);
    snprintf(buf.ptr, 32, "%d", n);
    buf.len = strlen(buf.ptr);
    buf.ptr[buf.len] = '\0';
    if (buf.len == net_write(fd, buf.ptr, buf.len)) {
        ssize_t readed;
        buf.len = 0;
        if (0 < (readed = net_read(fd, &buf, 1000))) {
            buf.ptr[buf.len] = '\0';
            printf("%s\n", buf.ptr);
        }
    }
    free(buf.ptr);
}

void *thread_1 (void *param) {
    int fd = net_connect("127.0.0.1", "9876", 0);
    if (-1 == fd)
        return NULL;
    for (int i = 0; i < task_count; ++i)
        inout(fd);
//    net_write(fd, CONST_STR_LEN("close"));
    close(fd);
    return NULL;
}

void *thread_2 (void *param) {
    for (int i = 0; i < task_count; ++i) {
        int fd = net_connect("127.0.0.1", "9876", 0);
        if (fd > 0) {
            inout(fd);
            close(fd);
        }
    }
    return NULL;
}

void test (void*(*fn)(void*)) {
    pthread_t pids [thread_count];
    for (int i = 0; i < thread_count; ++i)
        pthread_create(&pids[i], NULL, fn, NULL);
    for (int i = 0; i < thread_count; ++i)
        pthread_join(pids[i], NULL);
}

int main (int argc, const char *argv[]) {
    char *tail;
    srand(time(0));
    if (argc != 3) {
        printf("parameters ?\n");
        return 1;
    }
    thread_count = strtol(argv[1], &tail, 0);
    if ('\0' != *tail || ERANGE == errno) {
        printf("thread count ?\n");
        return 1;
    }
    task_count = strtol(argv[2], &tail, 0);
    if ('\0' != *tail || ERANGE == errno) {
        printf("task count ?\n");
        return 1;
    }
    test(thread_1);
//    test(thread_2);
    return 0;
}
