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
    switch(g_current_mod) {                                                   \
    case PING_STATE:                                                          \
        rv = ping_ ## fn (__VA_ARGS__);                                       \
        break;                                                                \
    case PONG_STATE:                                                          \
        rv = pong_ ## fn (__VA_ARGS__);                                       \
        break;                                                                \
    }                                                                         \
} while(0)

#define SWITCH_CALL_VOID(fn, ...) do {                                        \
    switch(g_current_mod) {                                                   \
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
    res = pong_process_message((uint8_t *)msg, msg->size);
    OUT_FUNC(sav);
    return res;
}

int pong_send (message_t *msg)
{
    int sav, res;
    IN_PING(sav);
    res = ping_process_message((uint8_t *)msg, msg->size);
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

struct call_storage {
    struct receivers receivers;
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
};

struct call_storage *g_pcall_storage = NULL;

#define TEST_CALL(F, ...) ({                                                \
    uint8_t TC__args [] = { __VA_ARGS__ };                                  \
    test_call(F, sizeof(TC__args), TC__args);                               \
})

int test_call(int f, uint16_t len, uint8_t args[])
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

int TEST_RECEIVER(call_storage_tests, ping, ts1) (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                     uint16_t vac, uint8_t vav[])
{
    ++g_pcall_storage->pi1_called;
    g_pcall_storage->pi1_args.a1 = AR1;
    g_pcall_storage->pi1_args.a2 = AR2;
    g_pcall_storage->pi1_args.a3 = AR3;
    g_pcall_storage->pi1_args.valen = vac;
    return g_pcall_storage->receivers.ping_rcv_ts1(AR1, AR2, AR3, vac, vav);
}
int TEST_RECEIVER(call_storage_tests, ping, ts2) (uint8_t AR1, uint8_t AR2,
                                     uint16_t vac, uint8_t vav[])
{
    ++g_pcall_storage->pi2_called;
    g_pcall_storage->pi2_args.a1 = AR1;
    g_pcall_storage->pi2_args.a2 = AR2;
    g_pcall_storage->pi2_args.valen = vac;
    return g_pcall_storage->receivers.ping_rcv_ts2(AR1, AR2, vac, vav);
}
int TEST_RECEIVER(call_storage_tests, pong, ts1)  (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                      uint16_t vac, uint8_t vav[])
{
    ++g_pcall_storage->po1_called;
    g_pcall_storage->po1_args.a1 = AR1;
    g_pcall_storage->po1_args.a2 = AR2;
    g_pcall_storage->po1_args.a3 = AR3;
    g_pcall_storage->po1_args.valen = vac;
    return g_pcall_storage->receivers.pong_rcv_ts1(AR1, AR2, AR3, vac, vav);
}
int TEST_RECEIVER(call_storage_tests, pong, ts2) (uint8_t AR1, uint8_t AR2,
                                     uint16_t vac, uint8_t vav[])
{
    ++g_pcall_storage->po2_called;
    g_pcall_storage->po2_args.a1 = AR1;
    g_pcall_storage->po2_args.a2 = AR2;
    g_pcall_storage->po2_args.valen = vac;
    return g_pcall_storage->receivers.pong_rcv_ts2(AR1, AR2, vac, vav);
}
TEST_ADD(call_storage_tests);

#define CS_TEST_RECEIVER(TEST, MOD, NAME) TEST ## _ ## MOD ## _cs_rcv_ ## NAME
#define CS_TEST_STORAGE(TEST) TEST ## _storage
#define CS_TEST_ADD(TEST) static struct call_storage CS_TEST_STORAGE(TEST) = { \
    .receivers = {                                                             \
        .ping_rcv_ts1 = CS_TEST_RECEIVER(TEST, ping, ts1),                     \
        .ping_rcv_ts2 = CS_TEST_RECEIVER(TEST, ping, ts2),                     \
        .pong_rcv_ts1 = CS_TEST_RECEIVER(TEST, pong, ts1),                     \
        .pong_rcv_ts2 = CS_TEST_RECEIVER(TEST, pong, ts2),                     \
    },                                                                         \
}

#define CS_TEST_INIT(TEST) do {                                                \
    TEST_INIT(call_storage_tests);                                             \
    g_pcall_storage = &CS_TEST_STORAGE(TEST);                                  \
    memset (&g_pcall_storage->pi1_called, 0,                                   \
            sizeof(*g_pcall_storage) - ((void *)&g_pcall_storage->pi1_called - \
                                        (void *)g_pcall_storage));             \
} while (0)

int CS_TEST_RECEIVER(CHAIN, ping, ts1) (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                        uint16_t vac, uint8_t vav[])
{
    return TEST_CALL(1, AR1, AR2, AR3);
}
int CS_TEST_RECEIVER(CHAIN, ping, ts2) (uint8_t AR1, uint8_t AR2,
                                        uint16_t vac, uint8_t vav[])
{
    return TEST_CALL(2, AR1, AR2);
}

int CS_TEST_RECEIVER(CHAIN, pong, ts1) (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                        uint16_t vac, uint8_t vav[])
{
    return TEST_CALL(2, AR1, AR2, 1);
}

int CS_TEST_RECEIVER(CHAIN, pong, ts2) (uint8_t AR1, uint8_t AR2,
                                        uint16_t vac, uint8_t vav[])
{
    return EOK;
}
CS_TEST_ADD(CHAIN);

static void test_chain(void **state)
{
    int sav;

    CS_TEST_INIT(CHAIN);

    IN_PONG(sav);
    assert_return_code(TEST_CALL(1, 1, 2, 3), 0);
    OUT_FUNC(sav);
    assert_int_equal(1, g_pcall_storage->pi1_called);
    assert_int_equal(1, g_pcall_storage->pi2_called);
    assert_int_equal(1, g_pcall_storage->po1_called);
    assert_int_equal(1, g_pcall_storage->po2_called);
}

int CS_TEST_RECEIVER(PARAMS, ping, ts1) (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                        uint16_t vac, uint8_t vav[])
{
    return EOK;
}
int CS_TEST_RECEIVER(PARAMS, ping, ts2) (uint8_t AR1, uint8_t AR2,
                                        uint16_t vac, uint8_t vav[])
{
    return EOK;
}

int CS_TEST_RECEIVER(PARAMS, pong, ts1) (uint8_t AR1, uint8_t AR2, uint8_t AR3,
                                        uint16_t vac, uint8_t vav[])
{
    return EOK;
}

int CS_TEST_RECEIVER(PARAMS, pong, ts2) (uint8_t AR1, uint8_t AR2,
                                        uint16_t vac, uint8_t vav[])
{
    return EOK;
}
CS_TEST_ADD(PARAMS);

static void test_params(void **state)
{
    int sav;

    CS_TEST_INIT(PARAMS);

#define STORAGE_VAL(FUNC, VAL) (g_current_mod == PING_STATE ?                 \
    g_pcall_storage->po ## FUNC ## _ ## VAL :                                 \
    g_pcall_storage->pi ## FUNC ## _ ## VAL)
#define TEST_CALL_CHECK_2(FUNC, ...) do {                                     \
    uint8_t __va[] = { __VA_ARGS__ };                                         \
    uint16_t __va_len = sizeof(__va);                                         \
    int __num_called = STORAGE_VAL(FUNC, called);                             \
    assert_int_not_equal(g_current_mod, OUTSIDE_STATE);                       \
    assert_return_code(TEST_CALL(FUNC, ## __VA_ARGS__), 0);                   \
    assert_int_equal(__va_len > 0 ? __va[0] : 0, STORAGE_VAL(FUNC, args.a1)); \
    assert_int_equal(__va_len > 1 ? __va[1] : 0, STORAGE_VAL(FUNC, args.a2)); \
    assert_int_equal(__va_len > 2 ? __va_len - 2 : 0,                         \
            STORAGE_VAL(FUNC, args.valen));                                   \
} while (0)
#define TEST_CALL_CHECK_3(FUNC, ...) do {                                     \
    uint8_t __va[] = { __VA_ARGS__ };                                         \
    uint16_t __va_len = sizeof(__va);                                         \
    int __num_called = STORAGE_VAL(FUNC, called);                             \
    assert_int_not_equal(g_current_mod, OUTSIDE_STATE);                       \
    assert_return_code(TEST_CALL(FUNC, ## __VA_ARGS__), 0);                   \
    assert_int_equal(STORAGE_VAL(FUNC, called), __num_called + 1);            \
    assert_int_equal(__va_len > 0 ? __va[0] : 0, STORAGE_VAL(FUNC, args.a1)); \
    assert_int_equal(__va_len > 1 ? __va[1] : 0, STORAGE_VAL(FUNC, args.a2)); \
    assert_int_equal(__va_len > 2 ? __va[2] : 0, STORAGE_VAL(FUNC, args.a3)); \
    assert_int_equal(__va_len > 3 ? __va_len - 3 : 0,                         \
            STORAGE_VAL(FUNC, args.valen));                                   \
} while (0)


    IN_PONG(sav);
    TEST_CALL_CHECK_3(1);
    TEST_CALL_CHECK_3(1, 0);
    TEST_CALL_CHECK_3(1, 0, 1);
    TEST_CALL_CHECK_3(1, 0, 1, 2);
    TEST_CALL_CHECK_3(1, 0, 1, 2, 3);

    TEST_CALL_CHECK_2(2);
    TEST_CALL_CHECK_2(2, 0);
    TEST_CALL_CHECK_2(2, 0, 1);
    TEST_CALL_CHECK_2(2, 0, 1, 2);
    TEST_CALL_CHECK_2(2, 0, 1, 2, 3);
    OUT_FUNC(sav);

    IN_PING(sav);
    TEST_CALL_CHECK_3(1);
    TEST_CALL_CHECK_3(1, 0);
    TEST_CALL_CHECK_3(1, 0, 1);
    TEST_CALL_CHECK_3(1, 0, 1, 2);
    TEST_CALL_CHECK_3(1, 0, 1, 2, 3);

    TEST_CALL_CHECK_2(2);
    TEST_CALL_CHECK_2(2, 0);
    TEST_CALL_CHECK_2(2, 0, 1);
    TEST_CALL_CHECK_2(2, 0, 1, 2);
    TEST_CALL_CHECK_2(2, 0, 1, 2, 3);
    OUT_FUNC(sav);

#undef TEST_CALL_CHECK_3
#undef TEST_CALL_CHECK_2
#undef STORAGE_VAL
}

int CS_TEST_RECEIVER(SUBSCRIPTION, ping, ts1) (uint8_t AR1, uint8_t AR2,
                                               uint8_t AR3,
                                               uint16_t vac, uint8_t vav[])
{
    int rc = remove_handlers(AR1, AR2, AR3);
    return rc < 0 ? rc : EOK;
}
int CS_TEST_RECEIVER(SUBSCRIPTION, ping, ts2) (uint8_t AR1, uint8_t AR2,
                                               uint16_t vac, uint8_t vav[])
{
    int rc = append_handler(vac, vav);
    return rc;
}

int CS_TEST_RECEIVER(SUBSCRIPTION, pong, ts1) (uint8_t AR1, uint8_t AR2,
                                               uint8_t AR3,
                                               uint16_t vac, uint8_t vav[])
{
    int rc = remove_handlers(AR1, AR2, AR3);
    return rc < 0 ? rc : EOK;
}

int CS_TEST_RECEIVER(SUBSCRIPTION, pong, ts2) (uint8_t AR1, uint8_t AR2,
                                               uint16_t vac, uint8_t vav[])
{
    return append_handler(vac, vav);
}

CS_TEST_ADD(SUBSCRIPTION);

static void test_subscription(void **state)
{
    int sav;
#define MAX_SUBSCRIPTION_ARGS 16
    uint8_t buf [ sizeof (event_handler_dsc_t) + MAX_SUBSCRIPTION_ARGS ];
    event_handler_dsc_t *handler = (event_handler_dsc_t *) (buf + 2);

    CS_TEST_INIT(SUBSCRIPTION);

    IN_PONG(sav);

#define ADD_ARG(ev, mt) do {                                                   \
    assert_in_range(handler->num_args, 0, MAX_SUBSCRIPTION_ARGS);              \
    handler->args[handler->num_args][0] = ev;                                  \
    handler->args[handler->num_args][1] = mt;                                  \
    ++handler->num_args;                                                       \
} while (0)

#define SUBSCRIBE_SEND(f) test_call(f,                                         \
        2 + sizeof(*handler) + (handler->num_args*2), buf)

    memset(buf, 0, sizeof (buf));
    handler->rcv_event = 1;
    handler->called_method = 1;
    ADD_ARG(2, 3);
    ADD_ARG(3, 2);
    ADD_ARG(1, 1);
    assert_return_code(SUBSCRIBE_SEND(2), 0);
    assert_int_equal(1, g_pcall_storage->pi2_called);
    assert_return_code(event_po1_send (0, 100, 3, 2), 0); /* Handled, do nothing */
    assert_int_equal(1, g_pcall_storage->pi1_called);
    assert_return_code(event_po1_send (0, 0, 1, 1), 0); /* Handled, delete */
    assert_int_equal(2, g_pcall_storage->pi1_called);
    assert_return_code(event_po1_send (0, 0, 1, 1), 0); /* Not handled */
    assert_int_equal(2, g_pcall_storage->pi1_called);

    assert_return_code(SUBSCRIBE_SEND(2), 0);
    assert_int_equal(2, g_pcall_storage->pi2_called);

    handler->rcv_event = 2;
    handler->num_args = 0;
    ADD_ARG(1, 2);
    ADD_ARG(2, 3);

    assert_return_code(SUBSCRIBE_SEND(2), 0);
    assert_int_equal(3, g_pcall_storage->pi2_called);

    assert_return_code(event_po1_send (0, 100, 3, 2), 0); /* Handled, do nothing */
    assert_int_equal(3, g_pcall_storage->pi1_called);

    assert_return_code(event_po2_send (0, 10, 10), 0); /* Handled, do nothing */
    assert_int_equal(4, g_pcall_storage->pi1_called);

    assert_return_code(event_po2_send (0, 1, 1), 0); /* Handled, removes 1st event */
    assert_int_equal(5, g_pcall_storage->pi1_called);

    assert_return_code(event_po1_send (0, 0, 1, 1), 0); /* Not handled */
    assert_int_equal(5, g_pcall_storage->pi1_called);

    assert_return_code(event_po2_send (0, 2, 0), 0); /* Handled, removes 2nd event */
    assert_int_equal(6, g_pcall_storage->pi1_called);

    assert_return_code(event_po2_send (0, 1, 1), 0); /* Not handled */
    assert_int_equal(6, g_pcall_storage->pi1_called);

    OUT_FUNC(sav);

#undef SUBSCRIBE_SEND
#undef ADD_ARG
#undef MAX_SUBSCRIPTION_ARGS
}

int main (void)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_empty),
        cmocka_unit_test(test_chain),
        cmocka_unit_test(test_params),
        cmocka_unit_test(test_subscription),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
