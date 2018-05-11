#include <stdio.h>
#include <task.h>

int i, t;

static void on_create_slot (void *x) {
    slot_t *slot = (slot_t*)x;
    printf("create slot %lu\n", slot->task->slots->len);
}

static void on_destroy_slot (void *x) {
    slot_t *slot = (slot_t*)x;
    printf("destroy slot %lu\n", slot->task->slots->len);
}

static void on_msg (void *str) {
    printf("%s\n", (char*)str);
    free(str);
    sleep(t);
}

static void on_info (void *x) {
    printf("%d: %lu\n", i++, time(0));
}

static void test_1 (long livingtime, int timeout) {
    task_t *task = task_create();
    task_setopt(task, TASK_MAXSLOTS, TASK_DEFAULT_SLOTS);
    if (livingtime > 0)
        task_setopt(task, TASK_LIVINGTIME, livingtime);
    task_setopt(task, TASK_CREATESLOT, on_create_slot);
    task_setopt(task, TASK_DESTROYSLOT, on_destroy_slot);
    t = timeout+1;
    task_start(task);
    for (int i = 0; i < 16; ++i) {
        char *str = malloc(16);
        if (timeout > 0)
            sleep(timeout);
        snprintf(str, 16, "i:%d", i);
        task_cast(task, on_msg, str);
    }
    sleep(t);
    task_destroy(task);
    sleep(timeout);
}

static void test_2 () {
    task_t *task = task_create();
    task_setopt(task, TASK_MSG, on_info);
    task_setopt(task, TASK_TIMEOUT, 1000);
    i = 0;
    task_start(task);
    sleep(12);
    task_destroy(task);
}

int main () {
    test_1(3, 1);
    test_2();
    return 0;
}
