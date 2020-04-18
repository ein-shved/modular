#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 1000000UL  // 1 MHz
#include <util/twi.h>
#include <stdio.h>

#include <light.h> /* generated */
#include <modular.h>
#include <tests/util/usart_printf.h>

#define MSG_MAX_LEN 256
uint8_t msg[MSG_MAX_LEN];
uint8_t msg_size=0;
uint8_t *msg_ptr=msg;

enum {
    MSG_EMPTY,
    MSG_STARTED,
    MSG_HAS_SIZE,
    MSG_RECEIVED,
} msg_status = MSG_EMPTY;

#define BLOCK_ID 0x1
#define SLAVE_ADDRESS 0x1
#define MASTER_ADDRESS 0x0
#define SLAVE_ID (BLOCK_ID <<8 | SLAVE_ADDRESS)
#define MASTER_ID (BLOCK_ID <<8 | MASTER_ADDRESS)

#define DBG(fmt, args...) printf( "[MASTER] " fmt "\n", ##args)

ISR(TWI_vect)
{
    switch(TW_STATUS) {
    case TW_REP_START:
    case TW_START:
        /* write SLA+R, SLA=0x01 */
        TWDR = (SLAVE_ADDRESS << 1) | TW_READ;
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;

    case TW_MR_SLA_ACK: /* SLA+R transmitted, ACK received */
        TWCR = _BV(TWINT) | _BV(TWEN) | _BV(TWIE) | _BV(TWEA);
        break;

    case TW_MR_DATA_ACK: /* data received, ACK returned */
        TWCR = _BV(TWEN) | _BV(TWIE)| _BV(TWEA);
        *msg_ptr = TWDR;
        ++msg_ptr;
        switch (msg_status) {
        case MSG_EMPTY:
            msg_status = MSG_STARTED;
            /* Fall through */
        case MSG_STARTED:
            if (msg_ptr - msg >= sizeof (message_t)) {
                msg_size = ((message_t *)msg)->size;
                msg_status = MSG_HAS_SIZE;
            } else {
                break;
            }
            /* Fall through */
        case MSG_HAS_SIZE:
            if (msg_ptr - msg >= msg_size) {
                msg_status = MSG_RECEIVED;
            } else {
                break;
            }
            /* Fall through */
        case MSG_RECEIVED:
            msg_ptr = msg;
            TWCR |= _BV(TWSTO);
            break;
        }
        TWCR |= _BV(TWINT);
        break;
    case TW_MR_DATA_NACK:
    case TW_MR_SLA_NACK:
        TWCR = _BV(TWSTO) |  _BV(TWEN) | _BV(TWIE) | _BV(TWINT);
        msg_status = MSG_EMPTY;
        break;
    }
}

#define MAX_ETABLE_SIZE 256
static uint8_t etable [MAX_ETABLE_SIZE] = { 0 };
static uint16_t etable_len = 0;
event_handler_t *event_table_getter(uint16_t *out_len, uint16_t *out_maxsize)
{
    *out_len = etable_len;
    *out_maxsize = MAX_ETABLE_SIZE;
    return (event_handler_t *) etable;
}
void event_table_update_len(uint16_t len)
{
    etable_len = len;
}
void event_table_purge_urgent(void)
{
    etable_len = 0;
}
int light_send_data(message_t *msg)
{
    return EUNSUPPORTED;
}
int process_event(event_handler_t *handler, event_t *event)
{
    return light_process_event (handler, event);
}

int method_lit_receive(uint8_t state, uint16_t vac, uint8_t vav[])
{
    DBG("Set light %s", state ? "ON" : "OFF");
    if (state) {
        PORTB |= _BV(1);
    } else {
        PORTB &= ~ _BV(1);
    }
    return 0;
}
void init_events()
{
    int res = create_handler(SLAVE_ID, 1, 1, handler_arg_pair(1, 1));
}
int main(void)
{
    init_usart_for_printf(F_CPU, 0);
    sei(); // enable global interrupt
    init_events();

    DDRB &= ~ _BV(0); /* PORTB[0] as input from slave */
    DDRB |= _BV(1); /* PORTB[1] as output to light */
    for (;;) {
        if ((PINB & _BV(0)) && (msg_status == MSG_EMPTY)) { // Slave wants to send us data
            DBG("Slave wants to say something");
            msg_status = MSG_STARTED;
            TWCR = _BV(TWSTA) | _BV(TWINT) |  _BV(TWEN) | _BV(TWIE);
        }
        if (msg_status == MSG_RECEIVED) {
            DBG("Received message of length %d", msg_size);
            printf("\t0x");
            for (int i =0; i< msg_size; ++i) {
                printf("%02hhx", msg[i]);
            }
            printf("\n");
            int res = light_process_message(msg, msg_size);
            if (res) {
                DBG("Failed to process slave's message (%d)", res);
            }
            TWCR = _BV(TWINT) | _BV(TWIE);
            msg_status = MSG_EMPTY;
            msg_size = 0;
        }
    }
}
