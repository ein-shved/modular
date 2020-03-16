#include <ping.h>
#include <pong.h>
#include <stdio.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>
#include <string.h>

#define EVENT_TABLE_SIZE 256
#define U8(C) ((uint8_t) C)

static enum {
    PING_STATE,
    PONG_STATE,
    OUTSIDE_STATE,
} g_current_mod = OUTSIDE_STATE;

#define IN_PING(sav) do { sav = g_current_mod; g_current_mod = PING_STATE; } while (0)
#define IN_PONG(sav) do { sav = g_current_mod; g_current_mod = PONG_STATE; } while (0)
#define OUT_FUNC(sav) do { g_current_mod = sav; } while (0)

#define SWITCH_CALL(fn, rv, ...) do {                                         \
    switch(g_current_mod) {                                                           \
    case PING_STATE:                                                          \
        rv = ping_ ## fn (__VA_ARGS__);                                       \
        break;                                                                \
    case PONG_STATE:                                                          \
        rv = pong_ ## fn (__VA_ARGS__);                                       \
        break;                                                                \
    }                                                                         \
} while(0)

#define SWITCH_CALL_VOID(fn, ...) do {                                        \
    switch(g_current_mod) {                                                           \
    case PING_STATE:                                                          \
        ping_ ## fn (__VA_ARGS__);                                            \
        break;                                                                \
    case PONG_STATE:                                                          \
        pong_ ## fn (__VA_ARGS__);                                            \
        break;                                                                \
    }                                                                         \
} while(0)

int ping_send (message_t *msg)
{
    int sav, res;
    IN_PONG(sav);
    res = pong_process_message((uint8_t *)msg);
    OUT_FUNC(sav);
    return res;
}

int pong_send (message_t *msg)
{
    int sav, res;
    IN_PING(sav);
    res = ping_process_message((uint8_t *)msg);
    OUT_FUNC(sav);
    return res;
}

static uint8_t ping_event_table [EVENT_TABLE_SIZE];
static uint16_t ping_event_table_len = 0;
static event_handler_t *ping_event_table_getter(uint16_t *out_len,
                                                uint16_t *out_maxsize)
{
    *out_len = ping_event_table_len;
    *out_maxsize = EVENT_TABLE_SIZE;
    return (event_handler_t *)ping_event_table;
}

static uint8_t pong_event_table [EVENT_TABLE_SIZE];
static uint16_t pong_event_table_len = 0;
static event_handler_t *pong_event_table_getter(uint16_t *out_len,
                                                uint16_t *out_maxsize)
{
    *out_len = pong_event_table_len;
    *out_maxsize = EVENT_TABLE_SIZE;
    return (event_handler_t *)pong_event_table;
}

event_handler_t *event_table_getter(uint16_t *out_len, uint16_t *out_maxsize)
{
    event_handler_t *table = NULL;
    SWITCH_CALL(event_table_getter, table, out_len, out_maxsize);
    return table;
}

void ping_event_table_update_len(uint16_t len)
{
    ping_event_table_len = len;
}

void pong_event_table_update_len(uint16_t len)
{
    pong_event_table_len = len;
}

void event_table_update_len(uint16_t len)
{
    SWITCH_CALL_VOID(event_table_update_len, len);
}

int process_event(event_handler_t *handler, event_t *event)
{
    int rv = EFATAL;
    SWITCH_CALL(process_event, rv, handler, event);
    return rv;
}

void event_table_purge_urgent(void)
{
    printf ("FATAL PURGING EVENT TABLE FOR %s SIDE\n",
                g_current_mod == PING_STATE ? "PING" : "PONG");
    event_table_update_len(0);
}

struct receivers {
    int (*ping_rcv_ts1) (uint8_t, uint8_t, uint8_t, uint16_t, uint8_t[]);
    int (*ping_rcv_ts2) (uint8_t, uint8_t, uint16_t, uint8_t[]);
    int (*pong_rcv_ts1) (uint8_t, uint8_t, uint8_t, uint16_t, uint8_t[]);
    int (*pong_rcv_ts2) (uint8_t, uint8_t, uint16_t, uint8_t[]);
} *preceivers;

int ping_rcv_ts1 (uint8_t AR1, uint8_t AR2, uint8_t AR3, uint16_t vac, uint8_t vav[])
{
    return preceivers->ping_rcv_ts1(AR1, AR2, AR3, vac, vav);
}
int ping_rcv_ts2 (uint8_t AR1, uint8_t AR2, uint16_t vac, uint8_t vav[])
{
    return preceivers->ping_rcv_ts2(AR1, AR2, vac, vav);
}
int pong_rcv_ts1 (uint8_t AR1, uint8_t AR2, uint8_t AR3, uint16_t vac, uint8_t vav[])
{
    return preceivers->pong_rcv_ts1(AR1, AR2, AR3, vac, vav);
}
int pong_rcv_ts2 (uint8_t AR1, uint8_t AR2, uint16_t vac, uint8_t vav[])
{
    return preceivers->pong_rcv_ts2(AR1, AR2, vac, vav);
}

#define TEST_RECEIVER(TEST, MOD, NAME) TEST ## _ ## MOD ## _rcv_ ## NAME
#define TEST_ADD(TEST) static struct receivers TEST ## _receivers = {       \
    .ping_rcv_ts1 = TEST_RECEIVER(TEST, ping, ts1),                         \
    .ping_rcv_ts2 = TEST_RECEIVER(TEST, ping, ts2),                         \
    .pong_rcv_ts1 = TEST_RECEIVER(TEST, pong, ts1),                         \
    .pong_rcv_ts2 = TEST_RECEIVER(TEST, pong, ts2),                         \
}

#define TEST_INIT(TEST) preceivers = &TEST ## _receivers

int TEST_RECEIVER(EMPTY, ping, ts1) (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                     uint16_t vac, uint8_t vav[])
{
    return EOK;
}
int TEST_RECEIVER(EMPTY, ping, ts2) (uint8_t AR1, uint8_t AR2,
                                     uint16_t vac, uint8_t vav[])
{
    return EOK;
}
int TEST_RECEIVER(EMPTY, pong, ts1)  (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                      uint16_t vac, uint8_t vav[])
{
    return EOK;
}
int TEST_RECEIVER(EMPTY, pong, ts2) (uint8_t AR1, uint8_t AR2,
                                     uint16_t vac, uint8_t vav[])
{
    return EOK;
}
TEST_ADD(EMPTY);

static void test_empty(void **state) {
    (void) g_current_mod; /* unused */

    TEST_INIT(EMPTY);
}

struct {
    int pi1_called;
    struct {
        uint8_t a1;
        uint8_t a2;
        uint8_t a3;
        uint16_t valen;
    } pi1_args;
    int pi2_called;
    struct {
        uint8_t a1;
        uint8_t a2;
        uint16_t valen;
    } pi2_args;
    int po1_called;
    struct {
        uint8_t a1;
        uint8_t a2;
        uint8_t a3;
        uint16_t valen;
    } po1_args;
    int po2_called;
    struct {
        uint8_t a1;
        uint8_t a2;
        uint16_t valen;
    } po2_args;
} test_chain_state;

#define TEST_CHAIN_CALL(F, ...) ({                                          \
    uint8_t TC__args [] = { __VA_ARGS__ };                                  \
    test_chain_call(F, sizeof(TC__args), TC__args);                         \
})

int test_chain_call(int f, uint16_t len, uint8_t args[])
{
    uint8_t buf[sizeof(method_t) + len];
    method_t *method = (method_t *)buf;
    va_list va;

    method->msg.msg = MSG_METHOD;
    method->msg.id = 0;
    method->msg.size = len + sizeof(method_t);
    method->method_id = f;

    memcpy(method->args, args, len);

    switch (g_current_mod) {
    case PING_STATE:
        return ping_send(&method->msg);
        break;
    case PONG_STATE:
        return pong_send(&method->msg);
        break;
    }
    return EINVALID;
}

int TEST_RECEIVER(CHAIN, ping, ts1) (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                     uint16_t vac, uint8_t vav[])
{
    ++test_chain_state.pi1_called;
    test_chain_state.pi1_args.a1 = AR1;
    test_chain_state.pi1_args.a2 = AR2;
    test_chain_state.pi1_args.a3 = AR3;
    test_chain_state.pi1_args.valen = vac;
    return TEST_CHAIN_CALL(1, AR1, AR2, AR3);
}
int TEST_RECEIVER(CHAIN, ping, ts2) (uint8_t AR1, uint8_t AR2,
                                     uint16_t vac, uint8_t vav[])
{
    ++test_chain_state.pi2_called;
    test_chain_state.pi2_args.a1 = AR1;
    test_chain_state.pi2_args.a2 = AR2;
    test_chain_state.pi2_args.valen = vac;
    return TEST_CHAIN_CALL(2, AR1, AR2);
}
int TEST_RECEIVER(CHAIN, pong, ts1)  (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                      uint16_t vac, uint8_t vav[])
{
    ++test_chain_state.po1_called;
    test_chain_state.po1_args.a1 = AR1;
    test_chain_state.po1_args.a2 = AR2;
    test_chain_state.po1_args.a3 = AR3;
    test_chain_state.po1_args.valen = vac;
    return TEST_CHAIN_CALL(2, AR1, AR2, 1);
}
int TEST_RECEIVER(CHAIN, pong, ts2) (uint8_t AR1, uint8_t AR2,
                                     uint16_t vac, uint8_t vav[])
{
    ++test_chain_state.po2_called;
    test_chain_state.po2_args.a1 = AR1;
    test_chain_state.po2_args.a2 = AR2;
    test_chain_state.po2_args.valen = vac;
    return EOK;
}
TEST_ADD(CHAIN);
static void test_chain(void **state)
{
    int rc = 0;
    int sav;
    TEST_INIT(CHAIN);

    memset(&test_chain_state, 0, sizeof (test_chain_state));
    IN_PONG(sav);
    rc = TEST_CHAIN_CALL(1, 1, 2, 3);
    OUT_FUNC(sav);
    assert_int_equal(0, rc);
    assert_int_equal(1, test_chain_state.pi1_called);
    assert_int_equal(1, test_chain_state.pi2_called);
    assert_int_equal(1, test_chain_state.po1_called);
    assert_int_equal(1, test_chain_state.po2_called);
}

int main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_empty),
        cmocka_unit_test(test_chain),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
