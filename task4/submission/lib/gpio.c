

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


    uint32_t shift = (uint32_t)pin * 4u;
    GPIOx->CFGLR &= ~(0x0Fu << shift);
    GPIOx->CFGLR |=  ((uint32_t)mode << shift);


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