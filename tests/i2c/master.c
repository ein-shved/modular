#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU 1000000UL  // 1 MHz
#include <util/delay.h>
#include <util/twi.h>

#include <modular.h>
#include <light.h> /* generated */

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

ISR(TWI_vect)
{
    // react on TWI status and handle different cases
    uint8_t status = TWSR & 0xFC; // mask-out the prescaler bits
    switch(status) {
    case TW_START:  // start transmitted
        // write SLA+R, SLA=0x01
        TWDR = (SLAVE_ADDRESS << 1) | TW_READ;
        TWCR &= ~_BV(TWSTA); // clear TWSTA
        break;

    case TW_MR_SLA_ACK: // SLA+R transmitted, ACK received
        TWCR &= ~(_BV(TWSTA) | _BV(TWSTO));
        break;

    case TW_MR_DATA_ACK: // data received, ACK returned
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
                TWCR |= _BV(TWSTA);
                break;
            }
            /* Fall through */
        case MSG_RECEIVED:
            msg_ptr = msg;
            TWCR |= _BV(TWSTO);  // write stop bit
            TWCR &= ~_BV(TWSTA);  // clear start bit
            break;
        }
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
int light_send_data(message_t *msg)
{
    return EUNSUPPORTED;
}
int process_event(event_handler_t *handler, event_t *event)
{
    light_process_event (handler, event);
}

int method_lit_receive(uint8_t state, uint16_t vac, uint8_t vav[])
{
    if (state) {
        PORTB |= _BV(1);
    } else {
        PORTB &= ~ _BV(1);
    }
}
int main(void)
{
    // TWI setup
    sei(); // enable global interrupt

    // TWI-ENable , TWI Interrupt Enable
    TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE);

    DDRB &= ~ _BV(0); // PORTB[0] as input;
    DDRB |= _BV(1); // PORTB[1] as output;
    // infinite loop
    for (;;)
    {
        if (PINB & 0x1 && msg_status == MSG_EMPTY) { // Slave wants to send us data
            msg_status = MSG_STARTED;
            TWCR |= _BV(TWSTA);
        }
        if (msg_status == MSG_RECEIVED) {
            light_process_message(msg, msg_size);
            msg_status = MSG_EMPTY;
            msg_size = 0;
        }
    }
}
