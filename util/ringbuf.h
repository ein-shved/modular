#ifndef UTIL_RINGBUF_H
#define UTIL_RINGBUF_H

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>

#ifndef RINGBUF_SIZE
#define RINGBUF_SIZE 1024
#endif

typedef struct {
    size_t size;
    size_t read_head;
    size_t read_len;
    size_t write_head;
    size_t write_len;
    uint8_t *buf;
} ringbuf_t;

/**
 * Initialise ring buffer.
 *
 * @param r ring buffer.
 * @param buf pointer to memory where data space allocated.
 * @param size memory size for ring buffer in bytes.
 *
 * @return 0 in case of success or -1 when buf or size is sero.
 */
int ringbuf_init(ringbuf_t *r, void *buf, size_t size);

/**
 * Notify ring buffer API about data length going to be stored withing current
 * write operation. This may move already written data to the free space at the
 * beginning of buffer. Use ringbuf_commit to finish current write operation, or
 * ringbuf_abort to abort it and free already written data.
 *
 * @param r ring buffer.
 * @param len data length going to be written withing current write operation.
 *
 * @return 0 in case of success allocation or -1 when there no enough space for
 *          data. You may want to call ringbuf_abort in last case to free
 *          already occupied space or wait for read operations.
 */
int ringbuf_learn_len(ringbuf_t *r, size_t len);
int ringbuf_write(ringbuf_t *r, uint8_t *data, size_t len);
int ringbuf_write_learned(ringbuf_t *r, uint8_t *data, size_t len);
int ringbuf_commit(ringbuf_t *r);
int ringbuf_abort(ringbuf_t *r);
int ringbuf_shift(ringbuf_t *r, size_t len);

static inline bool ringbuf_is_empty(ringbuf_t *r)
{
    return !(r->read_len > 0);
}

static inline bool ringbuf_can_read(ringbuf_t *r, size_t len)
{
    return (len > 0) && (len <= r->read_len);
}

static uint8_t *ringbuf_get_head(ringbuf_t *r)
{
    return ringbuf_is_empty(r) ? NULL : &r->buf[r->read_head];
}

static inline void ringbuf_reset(ringbuf_t *r)
{
    r->read_head = r->read_len = r->write_head = r->write_len = 0;
}

/* ringbuf_write8, ringbuf_write16, ringbuf_write32 */
#define RINGBUF_WRITE_BITS_DEF(N)                                       \
static inline int ringbuf_write ## N(ringbuf_t *r, uint ## N ## _t val) \
{                                                                       \
    return ringbuf_write(r, (uint8_t *)&val, sizeof(val));              \
}                                                                       \
                                                                        \
static inline int ringbuf_write ## N ## _learned (ringbuf_t *r,         \
                                             uint ## N ## _t val)       \
{                                                                       \
    return ringbuf_write_learned(r, (uint8_t *)&val, sizeof(val));      \
}

RINGBUF_WRITE_BITS_DEF(8);
RINGBUF_WRITE_BITS_DEF(16);
RINGBUF_WRITE_BITS_DEF(32);
//RINGBUF_WRITE_BITS_DEF(64);
#undef RINGBUF_WRITE_BITS_DEF

/* You should check, cat you do this before doing.
 *
 * ringbuf_read8, ringbuf_read16, ringbuf_read32 */
#define RINGBUF_READ_BITS_DEF(N)                                        \
static inline uint ## N ## _t ringbuf_read ## N(ringbuf_t *r)           \
{                                                                       \
    uint ## N ## _t val;                                                \
    memcpy (&val, ringbuf_get_head(r), sizeof(val));                    \
    ringbuf_shift(r, sizeof(val));                                      \
    return val;                                                         \
}
RINGBUF_READ_BITS_DEF(8);
RINGBUF_READ_BITS_DEF(16);
RINGBUF_READ_BITS_DEF(32);
//RINGBUF_READ_BITS_DEF(64);
#undef RINGBUF_READ_BITS_DEF

#endif /* UTIL_RINGBUF_H */
