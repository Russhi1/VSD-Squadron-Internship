/*
 * gpio.c — Advanced GPIO Library Implementation
 * VSDSquadron Mini (CH32V003F4U6)
 *
 * CFGLR register — 4 bits per pin (bits [1:0] = MODE, bits [3:2] = CNF):
 *   0x3 = push-pull output 50 MHz   (GPIO_OUTPUT)
 *   0x4 = floating input            (GPIO_INPUT)
 *   0x8 = input with pull resistor  (GPIO_INPUT_PU / GPIO_INPUT_PD)
 *         OUTDR bit = 1 → pull-up, OUTDR bit = 0 → pull-down
 *
 * Author: Rushil Rai
 */

#include "gpio.h"

/* Enable the APB2 clock for a GPIO port — must be done before any register access */
static void enable_clock(GPIO_TypeDef *port)
{
    if      (port == GPIOA) RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;
    else if (port == GPIOC) RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    else if (port == GPIOD) RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
}

void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    enable_clock(port);

    /* Clear the 4 config bits for this pin, then write the new mode */
    port->CFGLR &= ~(0xF << (4 * pin));

    if      (mode == GPIO_OUTPUT)   { port->CFGLR |= (0x3 << (4 * pin)); }
    else if (mode == GPIO_INPUT)    { port->CFGLR |= (0x4 << (4 * pin)); }
    else if (mode == GPIO_INPUT_PU) { port->CFGLR |= (0x8 << (4 * pin)); port->OUTDR |=  (1 << pin); }
    else if (mode == GPIO_INPUT_PD) { port->CFGLR |= (0x8 << (4 * pin)); port->OUTDR &= ~(1 << pin); }
}

/* Set or clear the OUTDR bit to drive the pin HIGH or LOW */
void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value)
{
    if (value) port->OUTDR |=  (1 << pin);
    else       port->OUTDR &= ~(1 << pin);
}

void gpio_set   (GPIO_TypeDef *port, uint8_t pin) { port->OUTDR |=  (1 << pin); }
void gpio_clear (GPIO_TypeDef *port, uint8_t pin) { port->OUTDR &= ~(1 << pin); }
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin) { port->OUTDR ^=  (1 << pin); }

/* Read the live voltage on the pin from the input data register (INDR) */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->INDR >> pin) & 0x1;
}

/*
 * Read pin multiple times with a short delay between samples.
 * If all samples agree the pin is stable — return that level.
 * If any sample differs the pin is still bouncing — return GPIO_DEBOUNCE_UNSTABLE.
 * Each inter-sample delay is ~40 µs at 24 MHz.
 */
uint8_t gpio_debounce_read(GPIO_TypeDef *port, uint8_t pin, uint8_t samples)
{
    uint8_t ref = gpio_read(port, pin);

    for (uint8_t i = 1; i < samples; i++)
    {
        for (volatile int j = 0; j < 1000; j++);   /* ~40 µs delay */
        if (gpio_read(port, pin) != ref) return GPIO_DEBOUNCE_UNSTABLE;
    }

    return ref;
}