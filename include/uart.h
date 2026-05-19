#ifndef UART_H
#define UART_H

#include <ch32v00x.h>

void uart_init(uint32_t baud);
void uart_putchar(char c);
void uart_print(const char *s);
void uart_println(const char *s);
void uart_print_uint(uint32_t n);

#endif