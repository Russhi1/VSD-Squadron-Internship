
#include "gpio.h"

/* Enable APB2 clock for the given port before accessing its registers */
static void enable_clock(GPIO_TypeDef *port)
{
    if      (port == GPIOA) RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;
    else if (port == GPIOC) RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    else if (port == GPIOD) RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
}

void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    enable_clock(port);
    port->CFGLR &= ~(0xF << (4 * pin));   /* clear 4 bits for this pin */

    if      (mode == GPIO_OUTPUT)   { port->CFGLR |= (0x3 << (4 * pin)); }
    else if (mode == GPIO_INPUT)    { port->CFGLR |= (0x4 << (4 * pin)); }
    else if (mode == GPIO_INPUT_PU) { port->CFGLR |= (0x8 << (4 * pin)); port->OUTDR |=  (1 << pin); }
    else if (mode == GPIO_INPUT_PD) { port->CFGLR |= (0x8 << (4 * pin)); port->OUTDR &= ~(1 << pin); }
}

void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value)
{
    if (value) port->OUTDR |=  (1 << pin);
    else       port->OUTDR &= ~(1 << pin);
}

void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR ^= (1 << pin);
}

/* Read live voltage from the input data register */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->INDR >> pin) & 0x1;
}

/*
 * Take 'samples' readings with ~40 µs between each.
 * If all agree → return stable level.
 * If any differ → return GPIO_DEBOUNCE_UNSTABLE (pin still bouncing).
 */
uint8_t gpio_debounce_read(GPIO_TypeDef *port, uint8_t pin, uint8_t samples)
{
    uint8_t ref = gpio_read(port, pin);
    for (uint8_t i = 1; i < samples; i++)
    {
        for (volatile int j = 0; j < 1000; j++);
        if (gpio_read(port, pin) != ref) return GPIO_DEBOUNCE_UNSTABLE;
    }
    return ref;
}