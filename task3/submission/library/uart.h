
#ifndef UART_H
#define UART_H

#include <ch32v00x.h>
#include <stdint.h>

void uart_init      (uint32_t baud);   /* configure USART1 at given baud rate  */
void uart_send_byte (uint8_t byte);    /* send one byte, blocks until TX ready  */
void uart_print     (const char *str); /* send null-terminated string           */
void uart_println   (const char *str); /* send string + CR+LF                   */
void uart_print_num (uint32_t n);      /* send unsigned integer as decimal text */
void uart_print_int (int32_t n);       /* send signed integer as decimal text   */

#endif /* UART_H */