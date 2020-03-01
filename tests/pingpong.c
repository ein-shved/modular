#include "ping.h"
#include "pong.h"
#include "stdio.h"

#define EVENT_TABLE_SIZE 256

static enum {
    PING_STATE,
    PONG_STATE,
    OUTSIDE_STATE,
} state = OUTSIDE_STATE;

#define IN_PING(sav) do { sav = state; state = PING_STATE; } while (0)
#define IN_PONG(sav) do { sav = state; state = PONG_STATE; } while (0)
#define OUT_FUNC(sav) do { state = sav; } while (0)

#define SWITCH_CALL(fn, rv, ...) do {                                         \
    switch(state) {                                                           \
    case PING_STATE:                                                          \
        rv = ping_ ## fn (__VA_ARGS__);                                       \
        break;                                                                \
    case PONG_STATE:                                                          \
        rv = pong_ ## fn (__VA_ARGS__);                                       \
        break;                                                                \
    }                                                                         \
} while(0)

#define SWITCH_CALL_VOID(fn, ...) do {                                        \
    switch(state) {                                                           \
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
                state == PING_STATE ? "PING" : "PONG");
    event_table_update_len(0);
}
int ping_rcv_ts1 (uint8_t AR1, uint8_t AR2, uint8_t AR3, uint16_t vac, uint8_t vav[])
{
    return EOK;
}
int ping_rcv_ts2 (uint8_t AR1, uint8_t AR2, uint16_t vac, uint8_t vav[])
{
    return EOK;
}
int pong_rcv_ts1 (uint8_t AR1, uint8_t AR2, uint8_t AR3, uint16_t vac, uint8_t vav[])
{
    return EOK;
}
int pong_rcv_ts2 (uint8_t AR1, uint8_t AR2, uint16_t vac, uint8_t vav[])
{
    return EOK;
}

int main (void)
{
}
