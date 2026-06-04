/* uart.h */
#ifndef UART_H
#define UART_H

#include <stdint.h>

/* Setup USART1 at the given baud rate. TX=PD5, RX=PD6. */
void    uart_init(uint32_t baud);

/* Transmit functions */
void    uart_print(const char *s);
void    uart_println(const char *s);
void    uart_print_num(uint32_t n);
void    uart_print_int(int32_t n);

/*
 * Non-blocking receive.
 * uart_rx_available() returns 1 if a byte is waiting in the RX buffer.
 * uart_read_byte()    reads and returns that byte (clears RXNE automatically).
 * Never call uart_read_byte() without checking uart_rx_available() first.
 */
uint8_t uart_rx_available(void);
char    uart_read_byte(void);

#endif /* UART_H */