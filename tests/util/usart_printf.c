#include "usart_printf.h"
#include <avr/io.h>

/* Code taken from
 *
 * https://efundies.com/avr-and-printf/
 */
#define BAUD 19200

static void usart_init(uint16_t ubrr);
static char usart_getchar( void );
static void usart_putchar( char data );
static void usart_pstr (char *s);
static unsigned char usart_kbhit(void);

static int usart_putchar_printf(char var, FILE *stream);


int init_usart_for_printf(unsigned long long f_cpu, unsigned long baud)
{
    static FILE usart = FDEV_SETUP_STREAM(
        usart_putchar_printf, NULL, _FDEV_SETUP_WRITE);

    if (baud == 0)
        baud = BAUD;
    stdout = &usart;
    usart_init (f_cpu/16/baud - 1);
    return 0;
}
static void usart_init( uint16_t ubrr)
{
    /* Set baud rate */
    UBRRH = (uint8_t)(ubrr>>8);
    UBRRL = (uint8_t)ubrr;
    /* Enable receiver and transmitter */
    UCSRB = (1<<RXEN)|(1<<TXEN);
    /* Set frame format: 8data, 1stop bit */
    UCSRC = (1<<URSEL)|(3<<UCSZ0);
}
static void usart_putchar(char data)
{
    while ( !(UCSRA & (_BV(UDRE))) );
    UDR = data;
}
static char usart_getchar(void)
{
    while ( !(UCSRA & (_BV(RXC))) );
    return UDR;
}
static unsigned char usart_kbhit(void)
{
    /* return nonzero if char waiting polled version */
    unsigned char b;
    b=0;
    if(UCSRA & (1<<RXC)) b=1;
    return b;
}
static void usart_pstr(char *s)
{
    while (*s) {
        usart_putchar(*s);
        s++;
    }
}

static int usart_putchar_printf(char var, FILE *stream)
{
    /*( translate \n to \r for br@y++ terminal */
    if (var == '\n') usart_putchar('\r');
    usart_putchar(var);
    return 0;
}
