#include <stdio.h>
#include <task.h>

int i;

static void on_msg (void *str, void *dummy) {
    printf("%s\n", (char*)str);
    free(str);
}

static void on_info (void *x, void *y) {
    printf("%d: %lu\n", i++, time(0));
}

static void test_1 () {
    printf("constant slots\n\n");
    task_t *task = task_create();
    task_setopt(task, TASK_MAXSLOTS, TASK_DEFAULT_SLOTS);
    task_start(task);
    for (int i = 0; i < 10; ++i) {
        char *str = malloc(16);
        snprintf(str, 16, "i:%d", i);
        task_cast(task, on_msg, str, NULL);
    }
    sleep(3);
    task_destroy(task);
}

static void test_2 () {
    printf("timeout\n\n");
    task_t *task = task_create();
    task_setopt(task, TASK_MSG, on_info);
    task_setopt(task, TASK_TIMEOUT, 1000);
    i = 0;
    task_start(task);
    sleep(12);
    task_destroy(task);
}

int main () {
    test_1();
    test_2();
    return 0;
}
