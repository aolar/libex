#include "msg.h"

void test_mpz (char *s) {
#ifdef __GMP__
    int32_t ni;
    strptr_t sp = { .ptr = s, .len = strlen(s) };
    msgbuf_t imsg = MSG_INIT, omsg = MSG_INIT;
    mpz_t z;
    mpz_init_set_str(z, s, 10);
    msg_create_request(&omsg, 0, CONST_STR_NULL, 64, 64);
    msg_seti32(&omsg, 5);
    msg_setstr(&omsg, sp.ptr, sp.len);
    msg_setmpz(&omsg, z);
    msg_seti32(&omsg, 7);
    mpz_clear(z);
    mpz_init(z);
    sp.ptr = NULL;
    sp.len = 0;
    msg_load_request(&imsg, omsg.ptr, omsg.len);
    msg_geti32(&imsg, &ni);
    msg_getstr(&imsg, &sp);
    msg_getmpz(&imsg, z);
    msg_geti32(&imsg, &ni);
    char *str = mpz_get_str(NULL, 10, z);
    printf("%s\n", str);
    mpz_clear(z);
    free(str);
    msg_clear(&omsg);
#endif
}


int main () {
    test_mpz("12345678987654321");
    return 0;
}
