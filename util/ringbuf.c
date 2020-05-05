#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "ringbuf.h"

int ringbuf_init(ringbuf_t *r, void *buf, size_t size)
{
    if (buf == NULL || size < 1) {
        return -1;
    }
    r->buf = buf;
    r->size = size;
    ringbuf_reset(r);

    return 0;
}

int ringbuf_learn_len(ringbuf_t *r, size_t len)
{
    if (r->write_head < r->read_head) { /* We already write to ring buffer
                                         * head - there no where to move */
        /* We may to have not strict comparison but in this case upon
         * commit the write_head becomes equal to read_head, which implies
         * as empty buffer - error state in this case. */
        return r->write_head + len < r->read_head ? 0 : -1;
    }
    /* else other */
    if (r->write_head + len <= r->size) {
        return 0;
    }
    if (len > r->read_head) {
        return -1;
    }
    memcpy(r->buf, &r->buf[r->write_head], r->write_len);
    return 0;
}
int ringbuf_write(ringbuf_t *r, uint8_t *data, size_t len)
{
    if (len == 0) {
        return 0;
    }
    if (ringbuf_learn_len(r, len) < 0) {
        return -1;
    }
    return ringbuf_write_learned(r, data, len);
}
int ringbuf_write_learned(ringbuf_t *r, uint8_t *data, size_t len)
{
    memcpy (&r->buf[r->write_head + r->write_len], data, len);
    r->write_len += len;
    return 0;
}

int ringbuf_commit(ringbuf_t *r)
{
    r->write_head += r->write_len;
    r->write_len = 0;
    if (r->write_head >= r->read_head + r->read_len) {
        r->read_len = r->write_head - r->read_head;
    } else if (r->write_head >= r->read_head) { /* Withing read space */
        /* Something went wrong */
        return -1;
    }
    return 0;
}
int ringbuf_abort(ringbuf_t *r)
{
    r->write_head = r->read_head + r->read_len;
    r->write_len = 0;
}
int ringbuf_shift(ringbuf_t *r, size_t len)
{
    if (!ringbuf_can_read(r, len)) {
        return -1;
    }
    r->read_head += len;
    r->read_len -= len;
    if (r->read_len == 0) {
        if(r->write_head + r->write_len < r->read_head) {
            /* The space in beginning was used */
            r->read_head = 0;
            r->read_len = r->write_head /* - (r->read_head = 0) */;
        } else {
            /* There no data committed, prepare read_head for future commit */
            r->read_head = r->write_head;
        }
    }
}
