#include "gpio.h"

static void enable_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;
    else if (port == GPIOC)
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    else if (port == GPIOD)
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
}

void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    enable_clock(port);

    port->CFGLR &= ~(0xF << (4 * pin));

    if (mode == GPIO_OUTPUT)
    {
        port->CFGLR |= (0x3 << (4 * pin));
    }
    else if (mode == GPIO_INPUT)
    {
        port->CFGLR |= (0x4 << (4 * pin));
    }
    else if (mode == GPIO_INPUT_PU)
    {
        port->CFGLR |= (0x8 << (4 * pin));
        port->OUTDR |= (1 << pin);
    }
    else if (mode == GPIO_INPUT_PD)
    {
        port->CFGLR |= (0x8 << (4 * pin));
        port->OUTDR &= ~(1 << pin);
    }
}

void gpio_set(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR |= (1 << pin);
}

void gpio_clear(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR &= ~(1 << pin);
}

void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR ^= (1 << pin);
}

uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->INDR >> pin) & 0x1;
}