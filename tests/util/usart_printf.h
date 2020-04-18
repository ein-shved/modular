#include <stdio.h>

#ifndef TESTS_UTIL_USART_PRINTF
#define TESTS_UTIL_USART_PRINTF

/**
 * Initialise uart to send printf text to it.
 * @param f_cpu cpu frequency (mostly - F_CPU macro)
 * @param baud desired baud rate or 0 for default 19200
 * @return 0
 */
int init_usart_for_printf(unsigned long long f_cpu, unsigned long baud);

#endif /* TESTS_UTIL_USART_PRINTF */
