#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 1000000UL  // 1 MHz
#include <util/delay.h>
#include <util/twi.h>

#include <button.h> /* generated */
#include <modular.h>

#define MSG_MAX_LEN 256
uint8_t *msg = NULL;
uint8_t msg_size=0;
uint8_t *msg_ptr = NULL;

enum {
    NO_MSG,
    MSG_READY,
    MSG_START,
    MSG_SENT,
} msg_status = NO_MSG;

#define BLOCK_ID 0x1
#define SLAVE_ADDRESS 0x1
#define MASTER_ADDRESS 0x0
#define SLAVE_ID (BLOCK_ID <<8 | SLAVE_ADDRESS)
#define MASTER_ID (BLOCK_ID <<8 | MASTER_ADDRESS)

ISR(TWI_vect)
{
  // react on TWI status and handle different cases
  uint8_t status = TWSR & 0xFC; // mask-out the prescaler bits
  switch(status)
  {
    case TW_ST_SLA_ACK:   // own SLA+T received, acknoledge sent
        if (msg_status == MSG_READY) {
            msg_status = MSG_START;
            PORTB &= ~ _BV(0);
        }
        /* Fall through */
    case TW_ST_DATA_ACK:
        if (msg_status == MSG_START) {
            if (msg_ptr - msg < msg_size) {
                TWDR = *msg_ptr++;
            } else {
                TWDR = 0;
            }
        }
        break;

    case TW_ST_LAST_DATA: // last byte transmitted ACK received
        msg_status = MSG_SENT;
        TWCR |= (1<<TWEA); // set TWEA to enter slave mode
    break;
    }
    TWCR |= (1<<TWINT);  // set TWINT -> activate TWI hardware
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
int button_send_data(message_t *in_msg)
{
    while (msg_status != NO_MSG);
    msg = (uint8_t *)in_msg;
    msg_size = in_msg->size;
    msg_status = MSG_READY;
    msg_ptr = msg;
    PORTB |= _BV(0);
    while (msg_status != MSG_SENT);
    return EOK;
}

int process_event(event_handler_t *handler, event_t *event)
{
    return button_process_event(handler, event);
}

int main(void)
{
    int state = 0, nstate;
    // TWI setup
    sei(); // enable global interrupt

    DDRB |= _BV(0); // PORTB[0] as output;
    DDRB &= ~ _BV(1); // PORTB[1] as input;

    // set slave address to 0x01, ignore general call
    TWAR = (SLAVE_ADDRESS << 1) | 0x00;
    // TWI-ENable , TWI Interrupt Enable
    TWCR |= _BV(TWEA) | _BV(TWEN) | _BV(TWIE);

    state = !!(PINB & _BV(1));
    event_btn_send(SLAVE_ID, state, state);
    // infinite loop
    for (;;)
    {
        nstate = !!(PINB & _BV(1));
        if (state != nstate) {
            event_btn_send(SLAVE_ID, nstate, state);
            state = nstate;
        }
    }
}
