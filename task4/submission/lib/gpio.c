

#include "gpio.h"

static void enable_clock(GPIO_TypeDef *port)
{
    if      (port == GPIOA) { RCC->APB2PCENR |= RCC_APB2Periph_GPIOA; }
    else if (port == GPIOC) { RCC->APB2PCENR |= RCC_APB2Periph_GPIOC; }
    else if (port == GPIOD) { RCC->APB2PCENR |= RCC_APB2Periph_GPIOD; }

}

void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    uint32_t shift = (uint32_t)pin * 4U;   /* bit position in CFGLR */

    /* Step 1 — power the port on */
    enable_clock(port);

    /* Step 2 — clear the 4 config bits for this pin */
    port->CFGLR &= ~(0x0FU << shift);

    /* Step 3 — write the new configuration */
    switch (mode) {

        case GPIO_MODE_OUTPUT:
         
            port->CFGLR |= (0x03U << shift);
            break;

        case GPIO_MODE_INPUT:
         
            port->CFGLR |= (0x04U << shift);
            break;

        case GPIO_MODE_INPUT_PU:
            port->CFGLR |= (0x08U << shift);
            port->OUTDR  |= (1U << pin);     /* OUTDR=1 → pull-up selected */
            break;

        case GPIO_MODE_INPUT_PD:
            port->CFGLR |= (0x08U << shift);
            port->OUTDR  &= ~(1U << pin);    /* OUTDR=0 → pull-down selected */
            break;

        default:
            break;
    }
}

void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t level)
{
    if (level == GPIO_HIGH) {
        port->OUTDR |=  (1U << pin);   /* drive pin to 3.3 V */
    } else {
        port->OUTDR &= ~(1U << pin);   /* drive pin to 0 V   */
    }
}

void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR ^= (1U << pin);
}
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (uint8_t)((port->INDR >> pin) & 0x01U);
}