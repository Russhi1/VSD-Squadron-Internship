/*
 * gpio.c — GPIO driver implementation for CH32V003
 *
 * Register map (from CH32V003 Reference Manual v1.9):
 *   GPIOA base: 0x40010800
 *   GPIOC base: 0x40011000
 *   GPIOD base: 0x40011400
 *
 *   Offset 0x00  GPIOx_CFGLR  — pin configuration (4 bits per pin)
 *   Offset 0x08  GPIOx_INDR   — input data register (read-only)
 *   Offset 0x10  GPIOx_BSHR  — bit set/reset (atomic)
 *   Offset 0x14  GPIOx_BCR   — bit reset only (atomic)
 *
 *   RCC->APB2PCENR:
 *     bit 2  IOPAEN
 *     bit 4  IOPCEN
 *     bit 5  IOPDEN
 */

#include "gpio.h"
#include <ch32v00x.h>

/* ── Helper: resolve GPIOx pointer from port index ─────────────────── */
static GPIO_TypeDef *_port_to_reg(uint8_t port)
{
    switch (port) {
        case PORT_A: return GPIOA;
        case PORT_C: return GPIOC;
        case PORT_D: return GPIOD;
        default:     return GPIOD;   /* safe fallback */
    }
}

/* ── Helper: enable the RCC clock for a port ───────────────────────── */
static void _enable_port_clock(uint8_t port)
{
    switch (port) {
        case PORT_A: RCC->APB2PCENR |= RCC_APB2Periph_GPIOA; break;
        case PORT_C: RCC->APB2PCENR |= RCC_APB2Periph_GPIOC; break;
        case PORT_D: RCC->APB2PCENR |= RCC_APB2Periph_GPIOD; break;
        default:     break;
    }
    /* Also enable AFIO clock; needed for any alternate-function or
     * external-interrupt mapping on this device. */
    RCC->APB2PCENR |= RCC_APB2Periph_AFIO;
}

/* ── Public API ─────────────────────────────────────────────────────── */

void gpio_init(uint8_t port, uint8_t pin, uint8_t mode)
{
    _enable_port_clock(port);

    GPIO_TypeDef *GPIOx = _port_to_reg(port);

    /*
     * GPIOx_CFGLR has 4 bits per pin (bits [4n+3 : 4n] for pin n).
     * Clear the 4-bit field, then OR in the new mode value.
     *
     * mode encoding used here:
     *   GPIO_MODE_OUTPUT      = 0x03  → CNF=00 MODE=11 (push-pull 30 MHz)
     *   GPIO_MODE_INPUT_FLOAT = 0x04  → CNF=01 MODE=00 (floating input)
     *   GPIO_MODE_INPUT_PU    = 0x08  → CNF=10 MODE=00 (pull-up/down)
     */
    uint32_t shift = (uint32_t)pin * 4u;
    GPIOx->CFGLR &= ~(0x0Fu << shift);
    GPIOx->CFGLR |=  ((uint32_t)mode << shift);

    /*
     * For pull-up input mode, writing 1 to the corresponding ODR bit
     * enables the internal pull-up (writing 0 would select pull-down).
     */
    if (mode == GPIO_MODE_INPUT_PU) {
        GPIOx->OUTDR |= (1u << pin);   /* select pull-up */
    }
}

void gpio_write(uint8_t port, uint8_t pin, uint8_t value)
{
    GPIO_TypeDef *GPIOx = _port_to_reg(port);
    if (value) {
        GPIOx->BSHR = (1u << pin);          /* atomic set   */
    } else {
        GPIOx->BCR  = (1u << pin);          /* atomic reset */
    }
}

uint8_t gpio_read(uint8_t port, uint8_t pin)
{
    GPIO_TypeDef *GPIOx = _port_to_reg(port);
    return (GPIOx->INDR >> pin) & 0x01u;
}