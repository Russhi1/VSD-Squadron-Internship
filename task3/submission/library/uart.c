/*
 * uart.c — UART Driver Library Implementation
 * VSDSquadron Mini (CH32V003F4U6)
 *
 * USART1 TX on PD5.
 * PD5 CFGLR: CNF=10, MODE=11 → AF push-pull 50 MHz → 0xB at bit 20.
 * BRR = HCLK / baud  →  24 000 000 / 115 200 = 208
 *
 * Author: Rushil Rai
 */

#include "uart.h"

void uart_init(uint32_t baud)
{
    /* Enable clocks for GPIOD and USART1 */
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;

    /* PD5 → AF push-pull 50 MHz (0xB at pin 5 = bit 20) */
    GPIOD->CFGLR &= ~(0xF << 20);
    GPIOD->CFGLR |=  (0xB << 20);

    /* Baud rate and enable */
    USART1->BRR   = (uint16_t)(SystemCoreClock / baud);
    USART1->CTLR1 = (1 << 13) | (1 << 3);   /* UE | TE */
}

void uart_send_byte(uint8_t byte)
{
    while (!(USART1->STATR & (1 << 7)));  /* wait for TXE */
    USART1->DATAR = byte;
}

void uart_print(const char *str)
{
    while (*str) uart_send_byte((uint8_t)*str++);
}

void uart_println(const char *str)
{
    uart_print(str);
    uart_send_byte('\r');
    uart_send_byte('\n');
}

void uart_print_num(uint32_t n)
{
    if (n == 0) { uart_send_byte('0'); return; }
    char buf[11]; int i = 0;
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (i--) uart_send_byte((uint8_t)buf[i]);
}

void uart_print_int(int32_t n)
{
    if (n < 0) { uart_send_byte('-'); uart_print_num((uint32_t)(-n)); }
    else       { uart_print_num((uint32_t)n); }
}