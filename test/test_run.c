#include "../include/libex/task.h"

void test_01 () {
    run_t r;
    memset(&r, 0, sizeof(run_t));
    run(CONST_STR_LEN("xterm -fg green -bg black"), &r, 0);
    printf("[%d]\n", r.pid);
    getchar();
    kill(r.pid, SIGTERM);
}

void test_02 () {
    char **cmd = mkcmdstr(CONST_STR_LEN("geth")), **p = cmd;
    while (*p)
        printf("%s ", *p++);
    printf("\n");
    free_cmd(cmd);
    cmd = p = mkcmdstr(CONST_STR_LEN("geth --rpc --rpcapi "));
    while (*p)
        printf("%s ", *p++);
    printf("\n");
    free_cmd(cmd);
    cmd = p = mkcmdstr(CONST_STR_LEN("geth --rpc --rpcapi \"admin,db,eth,debug,miner,net,shh,txpool,personal,web3\""));
    while (*p)
        printf("%s ", *p++);
    printf("\n");
    free_cmd(cmd);
    cmd = p = mkcmdstr(CONST_STR_LEN("geth --rpc --rpcapi \"admin,db,eth,debug,miner,net,shh,txpool,personal,web3\" --dev"));
    while (*p)
        printf("%s ", *p++);
    printf("\n");
    free_cmd(cmd);
}

int main (int argc, const char *argv[]) {
//    test_01();
    test_02();
    return 0;
}
