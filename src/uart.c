/*
 * uart.c — USART1 driver implementation for CH32V003
 *
 * Register map (CH32V003 Reference Manual v1.9, Section 12.10):
 *   USART1 base: 0x40013800
 *   +0x00 STATR  — status (TXE, TC, RXNE, …)
 *   +0x04 DATAR  — data (read = RDR, write = TDR), 9 bits wide
 *   +0x08 BRR    — baud rate: DIV_Mantissa[15:4] | DIV_Fraction[3:0]
 *   +0x0C CTLR1  — UE | M | TE | RE | TXEIE | RXNEIE …
 *
 * PD5 is USART1_TX (AF push-pull):  CFGLR CNF=10 MODE=11 → nibble 0xB
 * PD6 is USART1_RX (floating input): CFGLR CNF=01 MODE=00 → nibble 0x4
 *
 * Baud rate formula (RM Section 12.3):
 *   USARTDIV = HCLK / (16 * baud)
 *   BRR[15:4] = integer part, BRR[3:0] = fractional part (×16)
 *   Simplified: BRR = round(HCLK / baud) when using integer-only USARTDIV
 */

#include "uart.h"
#include <ch32v00x.h>

/* ── Internal: transmit one byte, blocking on TXE ─────────────────── */
static void _send(char c)
{
    while (!(USART1->STATR & USART_FLAG_TXE));
    USART1->DATAR = (uint16_t)(uint8_t)c;
}

/* ── Public API ─────────────────────────────────────────────────────── */

void uart_init(uint32_t baud)
{
    /* 1. Enable peripheral clocks */
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1
                    | RCC_APB2Periph_AFIO;

    /* 2. PD5 = TX: alternate-function push-pull output, 30 MHz
     *    nibble value = CNF[1:0]=10, MODE[1:0]=11 → 0xB           */
    GPIOD->CFGLR &= ~(0x0Fu << (5u * 4u));
    GPIOD->CFGLR |=  (0x0Bu << (5u * 4u));

    /* 3. PD6 = RX: floating input
     *    nibble value = CNF[1:0]=01, MODE[1:0]=00 → 0x4           */
    GPIOD->CFGLR &= ~(0x0Fu << (6u * 4u));
    GPIOD->CFGLR |=  (0x04u << (6u * 4u));

    /* 4. Baud rate divisor
     *    BRR = HCLK / baud  (integer rounding)                     */
    USART1->BRR = (uint16_t)((24000000UL + baud / 2u) / baud);

    /* 5. Enable USART, transmitter, and receiver (8N1 default)     */
    USART1->CTLR1 = USART_Mode_Tx | USART_Mode_Rx | ((uint16_t)0x2000u);
}

void uart_print(const char *s)
{
    while (*s) {
        _send(*s++);
    }
}

void uart_println(const char *s)
{
    uart_print(s);
    _send('\r');
    _send('\n');
}

void uart_print_num(uint32_t n)
{
    char buf[11];   /* max 10 decimal digits for uint32 + null */
    uint8_t i = 0u;
    if (n == 0u) { _send('0'); return; }
    while (n > 0u) {
        buf[i++] = (char)('0' + (n % 10u));
        n /= 10u;
    }
    /* digits are stored in reverse; print from back to front */
    while (i > 0u) {
        _send(buf[--i]);
    }
}

uint8_t uart_rx_available(void)
{
    return (USART1->STATR & USART_FLAG_RXNE) ? 1u : 0u;
}

char uart_read_byte(void)
{
    return (char)(USART1->DATAR & 0x1FFu);
}