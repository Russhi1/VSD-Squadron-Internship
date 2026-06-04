/* uart.c */
#include "uart.h"
#include <ch32v00x.h>

void uart_init(uint32_t baud)
{
    /* Enable clocks for GPIOD and USART1 */
    RCC->APB2PCENR |= RCC_APB2Periph_GPIOD | RCC_APB2Periph_USART1;

    /*
     * PD5 = TX: alternate-function push-pull output (CFGLR value 0xB)
     * CNF=10 (AF push-pull), MODE=11 (output 50MHz) → 4-bit field = 1011b = 0xB
     */
    GPIOD->CFGLR &= ~(0xFU << (5 * 4));
    GPIOD->CFGLR |=  (0xBU << (5 * 4));

    /*
     * PD6 = RX: floating input (CFGLR value 0x4)
     * CNF=01 (floating input), MODE=00 (input) → 4-bit field = 0100b = 0x4
     *
     * NOTE: PD6 is also the onboard LED. When UART RX is active,
     * PD6 belongs to USART1, not GPIO. Use PC0 for the application LED.
     */
    GPIOD->CFGLR &= ~(0xFU << (6 * 4));
    GPIOD->CFGLR |=  (0x4U << (6 * 4));

    /*
     * Baud rate register.
     * USARTDIV = HCLK / (16 * baud)
     * The +baud/2 rounds to nearest integer instead of truncating.
     */
    USART1->BRR = (uint16_t)((24000000UL + baud / 2) / baud);

    /* Enable transmitter, receiver, and the USART peripheral itself */
    USART1->CTLR1 = USART_CTLR1_TE | USART_CTLR1_RE | USART_CTLR1_UE;
}

static void _send_byte(char c)
{
    /* Wait until the TX data register is empty, then write */
    while (!(USART1->STATR & USART_STATR_TXE));
    USART1->DATAR = (uint16_t)c;
}

void uart_print(const char *s)
{
    while (*s) _send_byte(*s++);
}

void uart_println(const char *s)
{
    uart_print(s);
    _send_byte('\r');
    _send_byte('\n');
}

void uart_print_num(uint32_t n)
{
    char buf[11];   /* max 10 digits for uint32 + null */
    int  i = 0;
    if (n == 0) { _send_byte('0'); return; }
    while (n > 0) { buf[i++] = (char)('0' + n % 10); n /= 10; }
    while (i > 0) _send_byte(buf[--i]);
}

void uart_print_int(int32_t n)
{
    if (n < 0) { _send_byte('-'); uart_print_num((uint32_t)(-n)); }
    else        { uart_print_num((uint32_t)n); }
}

uint8_t uart_rx_available(void)
{
    /*
     * RXNE (Receive data register Not Empty) is set by hardware
     * when a complete byte has been received into USART1->DATAR.
     * It is cleared automatically when DATAR is read.
     */
    return (USART1->STATR & USART_STATR_RXNE) ? 1 : 0;
}

char uart_read_byte(void)
{
    return (char)(USART1->DATAR & 0x1FF);
}