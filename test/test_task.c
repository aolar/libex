#include <stdio.h>
#include <task.h>

int i, t;

static void on_create_slot (void *x) {
    slot_t *slot = (slot_t*)x;
    printf("create slot %lu\n", slot->pool->slots->len);
}

static void on_destroy_slot (void *x) {
    slot_t *slot = (slot_t*)x;
    printf("destroy slot %lu\n", slot->pool->slots->len);
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
    pool_t *pool = pool_create();
    pool_setopt(pool, POOL_MAXSLOTS, POOL_DEFAULT_SLOTS);
    if (livingtime > 0)
        pool_setopt(pool, POOL_LIVINGTIME, livingtime);
    pool_setopt(pool, POOL_CREATESLOT, on_create_slot);
    pool_setopt(pool, POOL_DESTROYSLOT, on_destroy_slot);
    t = timeout+1;
    pool_start(pool);
    for (int i = 0; i < 16; ++i) {
        char *str = malloc(16);
        if (timeout > 0)
            sleep(timeout);
        snprintf(str, 16, "i:%d", i);
        pool_call(pool, on_msg, str);
    }
    sleep(t);
    pool_destroy(pool);
    sleep(timeout);
}

static void test_2 () {
    pool_t *pool = pool_create();
    pool_setopt(pool, POOL_MSG, on_info);
    pool_setopt(pool, POOL_TIMEOUT, 1000);
    i = 0;
    pool_start(pool);
    sleep(12);
    pool_destroy(pool);
}

int main () {
    test_1(3, 1);
    test_2();
    return 0;
}
