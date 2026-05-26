/*
 * uart.h — UART Driver Library
 * VSDSquadron Mini (CH32V003F4U6)
 *
 * TX only on USART1, pin PD5.
 * Typical config: 115200 baud, 8N1.
 * Author: Rushil Rai
 */

#ifndef UART_H
#define UART_H

#include <ch32v00x.h>
#include <stdint.h>

/*
 * uart_init      — configure USART1 at the given baud rate (call once)
 * uart_send_byte — transmit a single byte (blocks until TX register empty)
 * uart_print     — transmit a null-terminated string
 * uart_println   — transmit a string followed by CR+LF
 * uart_print_num — transmit an unsigned integer as decimal text
 * uart_print_int — transmit a signed integer as decimal text
 */
void uart_init      (uint32_t baud);
void uart_send_byte (uint8_t byte);
void uart_print     (const char *str);
void uart_println   (const char *str);
void uart_print_num (uint32_t n);
void uart_print_int (int32_t n);

#endif /* UART_H */