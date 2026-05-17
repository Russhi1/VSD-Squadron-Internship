/*
 * gpio.h — GPIO Hardware Abstraction Layer
 * VSDSquadron Mini (CH32V003F4U6)
 *
 * Pin mapping source: VSDSquadron Mini DataSheet, Table 3
 *   Onboard LED  → PD6   (silkscreen label "PD6",  firmware pin 6, port GPIOD)
 *   Push button  → PD4   (silkscreen label "PD4",  firmware pin 4, port GPIOD)
 *   UART TX      → PD5   (silkscreen label "PD5",  firmware pin 5, port GPIOD)
 *   UART RX      → PD6   (shared with LED — do not use RX while LED is active)
 *
 * All GPIO pin numbers used here match the CH32V003 datasheet directly.
 * No custom numbering is invented.
 *
 * Author: Rushil Rai
 */

#ifndef GPIO_H
#define GPIO_H

#include <ch32v00x.h>

/* -----------------------------------------------------------------------
 * PORT ALIASES
 * The CH32V003F4U6 has three GPIO ports. We alias them to readable names.
 * DataSheet Table 2: 3 groups of GPIO ports, totalling 15 I/O ports.
 * ---------------------------------------------------------------------- */
#define PORT_A    GPIOA   /* Port A — PA1, PA2 (also ADC capable) */
#define PORT_C    GPIOC   /* Port C — PC1–PC7 (SPI, I2C, general) */
#define PORT_D    GPIOD   /* Port D — PD0–PD7 (UART, LED, button)  */

/* -----------------------------------------------------------------------
 * PIN MODE CONSTANTS
 * Used as the 'mode' argument to gpio_init().
 * Values map to the CFGLR register encoding in the CH32V003 manual:
 *   GPIO_OUTPUT  → CNF=00, MODE=11  (push-pull output, 50MHz)       = 0x3
 *   GPIO_INPUT   → CNF=01, MODE=00  (floating input, no resistor)   = 0x4
 *   GPIO_INPUT_PU→ CNF=10, MODE=00  (input + pull resistor, PU=1)   = 0x8
 *   GPIO_INPUT_PD→ CNF=10, MODE=00  (input + pull resistor, PD=0)   = 0x8
 * ---------------------------------------------------------------------- */
#define GPIO_OUTPUT      0
#define GPIO_INPUT       1
#define GPIO_INPUT_PU    2   /* pull-UP:   OUTDR bit = 1 */
#define GPIO_INPUT_PD    3   /* pull-DOWN: OUTDR bit = 0 */

/* -----------------------------------------------------------------------
 * LOGIC LEVEL ALIASES
 * ---------------------------------------------------------------------- */
#define GPIO_HIGH    1
#define GPIO_LOW     0

/* -----------------------------------------------------------------------
 * BOARD PIN SHORTCUTS  (DataSheet Table 3 — exact silkscreen labels)
 * ---------------------------------------------------------------------- */
#define LED_PIN    6   /* PD6 — 1x onboard user LED (DataSheet: "Built-in LED Pin") */
#define BTN_PIN    4   /* PD4 — onboard push button                                 */

/* -----------------------------------------------------------------------
 * FUNCTION PROTOTYPES
 * These four functions form the complete GPIO API for this firmware.
 * main.c calls ONLY these functions — no register access in main.c.
 * ---------------------------------------------------------------------- */

/*
 * gpio_init(port, pin, mode)
 *   Enables port clock and configures pin direction.
 *   Must be called before any other GPIO function on a pin.
 *
 *   Example:
 *     gpio_init(PORT_D, LED_PIN, GPIO_OUTPUT);
 */
void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode);

/*
 * gpio_set(port, pin)
 *   Drives pin HIGH (3.3V). Pin must be GPIO_OUTPUT.
 *   Equivalent to gpio_write(port, pin, GPIO_HIGH).
 *
 *   Example:
 *     gpio_set(PORT_D, LED_PIN);   // LED ON
 */
void gpio_set(GPIO_TypeDef *port, uint8_t pin);

/*
 * gpio_clear(port, pin)
 *   Drives pin LOW (0V). Pin must be GPIO_OUTPUT.
 *   Equivalent to gpio_write(port, pin, GPIO_LOW).
 *
 *   Example:
 *     gpio_clear(PORT_D, LED_PIN);  // LED OFF
 */
void gpio_clear(GPIO_TypeDef *port, uint8_t pin);

/*
 * gpio_toggle(port, pin)
 *   Flips pin: HIGH→LOW or LOW→HIGH. No state tracking needed.
 *   Uses XOR on the output data register.
 *
 *   Example:
 *     gpio_toggle(PORT_D, LED_PIN);  // blink
 */
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);

/*
 * gpio_read(port, pin)
 *   Returns 1 if pin is HIGH, 0 if LOW.
 *   Pin must be configured as an INPUT mode first.
 *
 *   Example:
 *     if (gpio_read(PORT_D, BTN_PIN) == GPIO_LOW) { ... }
 */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);

#endif /* GPIO_H */