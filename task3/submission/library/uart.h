

#ifndef UART_H
#define UART_H

#include <ch32v00x.h>
#include <stdint.h>

void uart_init      (uint32_t baud);
void uart_send_byte (uint8_t byte);
void uart_print     (const char *str);
void uart_println   (const char *str);
void uart_print_num (uint32_t n);
void uart_print_int (int32_t n);

#endif /* UART_H */