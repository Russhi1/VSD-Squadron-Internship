#include "uart.h"

void uart_init(uint32_t baud)
{
    RCC->APB2PCENR |= RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOD;
    GPIOD->CFGLR   &= ~(0xF << (4 * 5));
    GPIOD->CFGLR   |=  (0xB << (4 * 5));
    USART1->BRR     = (uint16_t)(24000000UL / baud);
    USART1->CTLR1   = USART_CTLR1_UE | USART_CTLR1_TE;
}

void uart_putchar(char c)
{
    while (!(USART1->STATR & USART_STATR_TXE));
    USART1->DATAR = (uint8_t)c;
}

void uart_print(const char *s)
{
    while (*s) uart_putchar(*s++);
}

void uart_println(const char *s)
{
    uart_print(s);
    uart_putchar('\r');
    uart_putchar('\n');
}

void uart_print_uint(uint32_t n)
{
    char buf[12];
    int i = 0;
    if (n == 0) { uart_putchar('0'); return; }
    while (n > 0) { buf[i++] = '0' + (n % 10); n /= 10; }
    while (--i >= 0) uart_putchar(buf[i]);
}