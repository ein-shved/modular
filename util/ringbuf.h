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
 * @brief Initialise ring buffer.
 *
 * @param r ring buffer.
 * @param buf pointer to memory where data space allocated.
 * @param size memory size for ring buffer in bytes.
 *
 * @returns 0 in case of success or -1 when buf or size is sero.
 */
int ringbuf_init(ringbuf_t *r, void *buf, size_t size);

/**
 * @brief Allocate space for data
 *
 * Notify ring buffer API about data length going to be stored withing current
 * write operation. This may move already written data to the free space at the
 * beginning of buffer. Use ringbuf_commit to finish current write operation, or
 * ringbuf_abort to abort it and free already written data.
 *
 * @param r ring buffer.
 * @param len data length going to be written withing current write operation.
 *
 * @returns 0 in case of success allocation or -1 when there no enough space for
 *          data. You may want to call ringbuf_abort in last case to free
 *          already occupied space or wait for read operations.
 */
int ringbuf_learn_len(ringbuf_t *r, size_t len);

/**
 * @brief Write checked data
 *
 * Writes len bytes of data to writing tail. This will not check the buffer
 * overflow. It expects you called ringbuf_learn_len with the same len already.
 *
 * @param r ring buffer
 * @param data pointer to data to store
 * @param len the data length
 *
 * @returns 0
 */
int ringbuf_write_learned(ringbuf_t *r, uint8_t *data, size_t len);

/**
 * @brief Check and write data
 *
 * This is a sequence of ringbuf_learn_len and ringbuf_write_learned.
 *
 * @param r ring buffer
 * @param data pointer to data to store
 * @param len the data length
 *
 * @returns 0 in case of success allocation or -1 when there no enough space for
 *          data. You may want to call ringbuf_abort in last case to free
 *          already occupied space or wait for read operations.
 */
int ringbuf_write(ringbuf_t *r, uint8_t *data, size_t len);

/**
 * @brief Save written data.
 *
 * After this operation, all data you previously write to buffer becomes
 * available for reading and you can not abort it.
 *
 * @param r ring buffer
 *
 * @returns 0 almost always or -1 in critical case, means a bug.
 */
int ringbuf_commit(ringbuf_t *r);

/**
 * @brief Drop written data.
 *
 * The data previously written after last commit are ignored and will nether
 * been read. Next write operations will overwrite them.
 *
 * @param r ring buffer
 */
void ringbuf_abort(ringbuf_t *r);

/**
 * @brief Free space.
 *
 * Use this to free space you do not longer needed after you copied data you
 * needed or processed it.
 *
 * @param r ring buffer
 * @param len bites to free
 *
 * @returns 0 wen all went ok, or -1 if there no len allocated bites
 */
int ringbuf_shift(ringbuf_t *r, size_t len);

/**
 * @brief Checks is buffer empty.
 *
 * @param r ring buffer
 *
 * @returns true if buffer is empty, false - otherwise.
 */
static inline bool ringbuf_is_empty(ringbuf_t *r)
{
    return !(r->read_len > 0);
}

/**
 * @brief Checks is len bites are available for read.
 *
 * @param r ring buffer
 * @param len requested data size
 *
 * @returns true if there are len bites available for reading, false -
 *          otherwise.
 */
static inline bool ringbuf_can_read(ringbuf_t *r, size_t len)
{
    return (len > 0) && (len <= r->read_len);
}

/**
 * @brief Get pointer to reading beginning.
 *
 * @param r ring buffer
 *
 * @returns pointer to reading head or NULL, if buffer is empty.
 */
static uint8_t *ringbuf_get_head(ringbuf_t *r)
{
    return ringbuf_is_empty(r) ? NULL : &r->buf[r->read_head];
}

/**
 * @brief Drop all data
 *
 * This resets buffer state and remove all previously read data. There no
 * data to read left in buffer after this operation. This may be useful after
 * critical data corruption (e.g. error withing commit)
 *
 * @param r ring buffer
 */
static inline void ringbuf_reset(ringbuf_t *r)
{
    r->read_head = r->read_len = r->write_head = r->write_len = 0;
}

/**
 * @brief Drop all committed data
 *
 * This is like reset, but softer. This drops only committed data, but current
 * writing operation will not be affected. Useful in case of minor data
 * corruption (e.g. you find out, that packet length in buffer is bigger then
 * available in buffer data)
 *
 * @param r ring buffer
 */
static inline void ringbuf_drop_readable(ringbuf_t *r)
{
    ringbuf_shift(r, r->read_len);
}

/**
 * @brief Helper to write and commit data
 *
 * @param r ring buffer
 * @param data pointer to data to store
 * @param len the data length
 *
 * @returns 0 in case of success allocation or -1 when there no enough space for
 *          data. You may want to call ringbuf_abort in last case to free
 *          already occupied space or wait for read operations.
 */
static inline int ringbuf_write_commit(ringbuf_t *r, uint8_t *data, size_t len)
{
    int rc = ringbuf_write(r, data, len);

    if (!rc) {
        rc = ringbuf_commit(r);
    }

    return rc;
}

/**
 * @brief Simplify writing words
 *
 * ringbuf_write8, ringbuf_write16, ringbuf_write32
 * ringbuf_write8_learned, ringbuf_write16_learned, ringbuf_write32_learned
 * ringbuf_write8_commit, ringbuf_write16_commit, ringbuf_write32_commit
 *
 * @see ringbuf_write, @see ringbuf_write_learned, @see ringbuf_write_commit
 */
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
}                                                                       \
                                                                        \
static inline int ringbuf_write ## N ## _commit (ringbuf_t *r,          \
                                             uint ## N ## _t val)       \
{                                                                       \
    return ringbuf_write_commit(r, (uint8_t *)&val, sizeof(val));       \
}

RINGBUF_WRITE_BITS_DEF(8);
RINGBUF_WRITE_BITS_DEF(16);
RINGBUF_WRITE_BITS_DEF(32);
//RINGBUF_WRITE_BITS_DEF(64);
#undef RINGBUF_WRITE_BITS_DEF

/**
 * @brief Simplify reading words.
 *
 * This functions shifts data.
 *
 * ringbuf_read8, ringbuf_read16, ringbuf_read32
 * ringbuf_read8_nocheck, ringbuf_read16_nocheck, ringbuf_read32_nocheck
 *
 * @see ringbuf_get_head, @see ringbuf_shift
 */
#define RINGBUF_READ_BITS_DEF(N)                                          \
static inline uint ## N ## _t ringbuf_read ## N ## _nocheck(ringbuf_t *r) \
{                                                                         \
    uint ## N ## _t val;                                                  \
    memcpy (&val, ringbuf_get_head(r), sizeof(val));                      \
    ringbuf_shift(r, sizeof(val));                                        \
    return val;                                                           \
}                                                                         \
static inline uint ## N ## _t ringbuf_read ## N(ringbuf_t *r)             \
{                                                                         \
    if (!ringbuf_can_read(r, sizeof(uint ## N ## _t))) {                  \
        return 0;                                                         \
    }                                                                     \
    return ringbuf_read ## N ## _nocheck(r);                              \
}
RINGBUF_READ_BITS_DEF(8);
RINGBUF_READ_BITS_DEF(16);
RINGBUF_READ_BITS_DEF(32);
//RINGBUF_READ_BITS_DEF(64);
#undef RINGBUF_READ_BITS_DEF

#endif /* UTIL_RINGBUF_H */
