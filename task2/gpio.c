/*
 * gpio.c — GPIO Hardware Abstraction Layer Implementation
 * VSDSquadron Mini (CH32V003F4U6)
 *
 * This file contains the register-level implementation of the GPIO API.
 * main.c never touches registers directly — all hardware access is here.
 *
 * Register map (CH32V003 reference manual):
 *   RCC->APB2PCENR  — clock enable register; GPIO ports are OFF by default
 *   GPIOx->CFGLR    — configuration register, 4 bits per pin (pins 0–7)
 *   GPIOx->OUTDR    — output data register (write a pin HIGH or LOW)
 *   GPIOx->INDR     — input  data register (read the live pin state)
 *
 * Author: Rushil Rai
 */

#include "gpio.h"

/* =======================================================================
 * INTERNAL HELPER: enable_clock(port)
 *
 * Why this is needed:
 *   The CH32V003 powers down all peripheral clocks on reset to save power.
 *   If you try to write to a GPIO register before its clock is on, the
 *   write is silently ignored — the pin never changes. This is a very
 *   common bug in bare-metal firmware.
 *
 *   We fix it by calling this function at the start of gpio_init().
 *   Using |= means we only SET bits — we never accidentally turn off a
 *   clock that was already enabled for another pin on the same port.
 * ======================================================================= */
static void enable_clock(GPIO_TypeDef *port)
{
    if (port == GPIOA)
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOA;
    else if (port == GPIOC)
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOC;
    else if (port == GPIOD)
        RCC->APB2PCENR |= RCC_APB2Periph_GPIOD;
}

/* =======================================================================
 * gpio_init(port, pin, mode)
 *
 * How the CFGLR register works:
 *   Each of the 8 pins (0–7) in a port gets exactly 4 bits in CFGLR.
 *   Pin N occupies bits [(4N+3) : (4N)].
 *   These 4 bits encode CNF[1:0] and MODE[1:0]:
 *
 *   CFGLR value  Binary   Meaning
 *   ───────────  ───────  ─────────────────────────────────────────
 *   0x3          0011     Push-pull output, 50MHz drive → GPIO_OUTPUT
 *   0x4          0100     Floating input (no resistor)  → GPIO_INPUT
 *   0x8          1000     Input with pull resistor      → GPIO_INPUT_PU / GPIO_INPUT_PD
 *                         (direction controlled by OUTDR bit:
 *                          OUTDR bit = 1 → pull-UP, OUTDR bit = 0 → pull-DOWN)
 *
 * Steps every gpio_init() call follows:
 *   1. Enable the port clock (safe to repeat; uses |=)
 *   2. Clear the 4 bits for this pin (always; avoids stale config)
 *   3. Write the new 4-bit mode value
 *   4. For pull-UP/DOWN modes: also set or clear the OUTDR bit
 * ======================================================================= */
void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    /* Step 1: power on the GPIO port clock */
    enable_clock(port);

    /* Step 2: clear the 4 config bits for this pin */
    port->CFGLR &= ~(0xF << (4 * pin));

    /* Step 3: write new configuration */
    if (mode == GPIO_OUTPUT)
    {
        /* 0x3 = push-pull output, 50MHz slew rate
         * Push-pull means the pin can both source and sink current — it
         * actively drives HIGH and actively drives LOW. */
        port->CFGLR |= (0x3 << (4 * pin));
    }
    else if (mode == GPIO_INPUT)
    {
        /* 0x4 = floating input — the pin is disconnected from both
         * supply rails and just measures the external voltage. */
        port->CFGLR |= (0x4 << (4 * pin));
    }
    else if (mode == GPIO_INPUT_PU)
    {
        /* 0x8 = input with pull resistor enabled */
        port->CFGLR |= (0x8 << (4 * pin));
        /* Step 4: OUTDR = 1 → internal resistor pulls pin UP to 3.3V */
        port->OUTDR |= (1 << pin);
    }
    else if (mode == GPIO_INPUT_PD)
    {
        /* 0x8 = input with pull resistor enabled */
        port->CFGLR |= (0x8 << (4 * pin));
        /* Step 4: OUTDR = 0 → internal resistor pulls pin DOWN to GND */
        port->OUTDR &= ~(1 << pin);
    }
}

/* =======================================================================
 * gpio_set(port, pin)
 *
 * Drives the pin HIGH (3.3V).
 * OUTDR (Output Data Register): bit N controls pin N.
 * |= sets bit N without touching any other bits.
 * ======================================================================= */
void gpio_set(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR |= (1 << pin);   /* set bit → pin HIGH (3.3V) */
}

/* =======================================================================
 * gpio_clear(port, pin)
 *
 * Drives the pin LOW (0V / GND).
 * &= ~(mask) clears bit N without touching any other bits.
 * ======================================================================= */
void gpio_clear(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR &= ~(1 << pin);  /* clear bit → pin LOW (0V) */
}

/* =======================================================================
 * gpio_toggle(port, pin)
 *
 * Flips the pin state: HIGH→LOW or LOW→HIGH.
 * XOR (^=) is the classic toggle trick:
 *   1 XOR 1 = 0  (was HIGH, becomes LOW)
 *   0 XOR 1 = 1  (was LOW,  becomes HIGH)
 * Only the target bit is affected; all other bits are XORed with 0,
 * so they are unchanged.
 * ======================================================================= */
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR ^= (1 << pin);
}

/* =======================================================================
 * gpio_read(port, pin)
 *
 * Returns the live logic state of a pin (1 = HIGH, 0 = LOW).
 * INDR (Input Data Register): bit N holds the current state of pin N.
 * We shift right by pin to bring bit N to position 0, then AND with 1
 * to isolate it.
 * ======================================================================= */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (port->INDR >> pin) & 0x1;
}