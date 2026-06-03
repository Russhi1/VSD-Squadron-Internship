/*
 * uart.h — USART1 driver for CH32V003
 *
 * Configures USART1 on PD5 (TX) / PD6 (RX) for 8N1 asynchronous
 * serial communication.  Transmit is blocking (spins on TXE).
 * Receive is non-blocking (caller polls uart_rx_available()).
 */

#ifndef UART_H
#define UART_H

#include <stdint.h>

/**
 * @brief  Initialise USART1 at the requested baud rate.
 * @param  baud  Baud rate in bits/s (e.g. 115200).
 *
 * Assumes HCLK = 24 MHz.  Divisor = (24_000_000 + baud/2) / baud.
 * PD5 is configured as push-pull alternate-function output (TX).
 * PD6 is configured as floating input (RX).
 */
void uart_init(uint32_t baud);

/**
 * @brief  Transmit a null-terminated string (blocking).
 * @param  s  Pointer to string; must not be NULL.
 */
void uart_print(const char *s);

/**
 * @brief  Transmit a string followed by CR+LF (blocking).
 */
void uart_println(const char *s);

/**
 * @brief  Transmit an unsigned 32-bit decimal number (blocking).
 * @param  n  Value to print; "0" is printed for n == 0.
 */
void uart_print_num(uint32_t n);

/**
 * @brief  Returns 1 if a received byte is waiting in USART1->DATAR.
 * @return 1 = byte available, 0 = nothing waiting.  Non-blocking.
 */
uint8_t uart_rx_available(void);

/**
 * @brief  Read and return the next received byte.
 *         Reading DATAR automatically clears the RXNE flag.
 * @return Received character.
 * @note   Only call after uart_rx_available() returns 1.
 */
char uart_read_byte(void);

#endif /* UART_H */