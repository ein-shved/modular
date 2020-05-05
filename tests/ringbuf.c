#include <util/ringbuf.h>

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#define MAX_RINGBUF_BUF_SIZE  (4 * RINGBUF_SIZE)

#define MAX(a,b) (((a) > (b)) ? (a) : (b))

struct ringbuf_test_err_dsc {
    ringbuf_t r;
    int err_n;
};

static int ringbuf_setup_err(void **state, size_t size)
{
    static uint8_t overall_data[MAX_RINGBUF_BUF_SIZE];
    static struct ringbuf_test_err_dsc test = { 0 };
    int i = 0, s = 0;

    while (s <= size) {
        i+=4;
        s+=i;
    }
    test.err_n = (i/4) - 1;
    ringbuf_init(&test.r, overall_data, size);
    *state = &test;
    return 0;
}

#define SETUP_F_ERR(N) ringbuf_setup_err ## N
#define SETUP_F_ERR_DEF(N) static int SETUP_F_ERR(N) (void **state)     \
{                                                                       \
    return ringbuf_setup_err(state, N);                              \
}
/*!/bin/bash
 *
 * i=0;
 * s=0;
 * while [ $s -lt N ]; do
 *      i=$(( $i + 4 ));
 *      s=$(( $s + $i ));
 *      echo "$(($i / 4 )): $i - $s";
 * done
 */
//SETUP_F_ERR_DEF(4, 1);
SETUP_F_ERR_DEF(16);
SETUP_F_ERR_DEF(256);
SETUP_F_ERR_DEF(263);
SETUP_F_ERR_DEF(264);
SETUP_F_ERR_DEF(265);

#define LEN_LEARN_UNKNOWN 0
#define LEN_LEARN_SECOND  1
#define LEN_LEARN_BEGIN   2
static void ringbuf_test_err(void **state)
{
    struct ringbuf_test_err_dsc *test = (struct ringbuf_test_err_dsc *) *state;
    ringbuf_t *r = &test->r;
    int err_n = test->err_n;
    int rc = 0;

    int learn_flag = LEN_LEARN_UNKNOWN;

#define CHECK_RC {                          \
    if (err_n >= i / 4) {                   \
        assert_int_equal(0, rc);            \
    } else if (learned && err_n < i / 4) {  \
        assert_int_not_equal(0, rc);        \
    }                                       \
    if (rc)                                 \
        break;                              \
}

    /* Write full */
    for (uint32_t i=4; ; i+=4) {
        int learned = 0;
        if (learn_flag == LEN_LEARN_BEGIN) {
            rc = ringbuf_learn_len(r, i);
            learned = 1;
            CHECK_RC;
        }
        for (uint16_t j=0; j < i/4; ++j) {
            rc = ringbuf_write32(r, i);
            CHECK_RC;
            if (learn_flag == LEN_LEARN_SECOND && !learned) {
                rc = ringbuf_learn_len(r, i);
                learned = 1;
                CHECK_RC;
            }
        }
        if (rc) {
            ringbuf_abort(r);
            break;
        }
        if (learn_flag == LEN_LEARN_UNKNOWN) {
            learned = 1;
            CHECK_RC;
        }
        rc = ringbuf_commit(r);
        assert_int_equal(0, rc);
        if (rc)
            break;
        learn_flag = (learn_flag + 1) % 3;
    }
    /* Read to middle */
    for (uint32_t i=4; i / 4 <= err_n/2; i+=4) {
        assert_true(ringbuf_can_read(r, sizeof (i)));
        if(!ringbuf_can_read(r, sizeof (i))) {
            break;
        }
        assert_int_equal(i, ringbuf_read32(r));
        for(uint16_t j=1; j < i / 4; ++j) {
            assert_true(ringbuf_can_read(r, sizeof(i)));
            assert_int_equal(i, ringbuf_read32(r));
        }
    }
#define CHECK_RC_HALF {                     \
    if (err_n >= i / 4) {                   \
        assert_int_equal(0, rc);            \
    }                                       \
    if (rc)                                 \
        break;                              \
}
    /* Write (half - 1) words */
    err_n = MAX(err_n/2, 1);
    for (uint32_t i = 4; err_n >= i/4 ; i+=4) {
        int learned = 0;
        if (learn_flag == LEN_LEARN_BEGIN) {
            rc = ringbuf_learn_len(r, i);
            learned = 1;
            CHECK_RC;
        }
        for (uint16_t j=0; j < i/4; ++j) {
            rc = ringbuf_write32(r, i);
            CHECK_RC;
            if (learn_flag == LEN_LEARN_SECOND && !learned) {
                rc = ringbuf_learn_len(r, i);
                learned = 1;
                CHECK_RC;
            }
        }
        if (rc) {
            ringbuf_abort(r);
            break;
        }
        if (learn_flag == LEN_LEARN_UNKNOWN) {
            learned = 1;
            CHECK_RC;
        }
        rc = ringbuf_commit(r);
        assert_int_equal(0, rc);
        if (rc)
            break;
        learn_flag = (learn_flag + 1) % 3;
    }
    /* Read second half */
    err_n = test->err_n;
    for (uint32_t i=MAX((err_n/2 + 1)*4, 4); i <= err_n * 4; i+=4) {
        assert_true(ringbuf_can_read(r, sizeof (i)));
        assert_int_equal(i, ringbuf_read32(r));
        for(uint16_t j=1; j < i / 4; ++j) {
            assert_true(ringbuf_can_read(r, sizeof(i)));
            assert_int_equal(i, ringbuf_read32(r));
        }
    }
#define CHECK_RC_READ {                     \
    if (err_n >= i / 4) {                   \
        assert_int_equal(0, rc);            \
    } else if (err_n < i / 4) {             \
        assert_int_not_equal(0, rc);        \
    }                                       \
    if (rc)                                 \
        break;                              \
}
    /* Read last part */
    err_n = MAX(err_n/2, 1);
    for (uint32_t i = 4; ; i += 4) {
        rc = ringbuf_can_read(r, sizeof (i)) ? 0 : -1;
        CHECK_RC_READ;
        assert_int_equal(i, ringbuf_read32(r));
        for(uint16_t j=1; j < i / 4; ++j) {
            rc = ringbuf_can_read(r, sizeof(i)) ? 0 : -1;
            CHECK_RC_READ;
            assert_int_equal(i, ringbuf_read32(r));
        }
        if (rc)
            break;
    }
    assert_true(ringbuf_is_empty(r));
}

int main (void)
{
    const struct CMUnitTest tests[] = {
//        cmocka_unit_test_setup(ringbuf_test_err, SETUP_F_ERR(4)),
        cmocka_unit_test_setup(ringbuf_test_err, SETUP_F_ERR(16)),
        cmocka_unit_test_setup(ringbuf_test_err, SETUP_F_ERR(256)),
        cmocka_unit_test_setup(ringbuf_test_err, SETUP_F_ERR(263)),
        cmocka_unit_test_setup(ringbuf_test_err, SETUP_F_ERR(264)),
        cmocka_unit_test_setup(ringbuf_test_err, SETUP_F_ERR(265)),
    };
    return cmocka_run_group_tests(tests, NULL, NULL);
}
