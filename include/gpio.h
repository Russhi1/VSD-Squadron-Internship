/*
 * gpio.h — GPIO Hardware Abstraction Library
 * VSD Squadron Mini (CH32V003F4U6)
 *
 * Provides a clean, register-free API for configuring and controlling
 * GPIO pins on the CH32V003.
 *
 * Supported ports: GPIOA, GPIOC, GPIOD
 * Supported modes: push-pull output, floating input,
 *                  pull-up input, pull-down input
 *
 * All functions are safe to call multiple times on the same pin.
 * Clock enabling is idempotent (uses |= so repeated calls are harmless).
 */

#ifndef GPIO_H
#define GPIO_H

/* Pull in the CH32V003 peripheral register definitions.
 * This gives us: GPIOA, GPIOC, GPIOD, GPIO_TypeDef,
 *                RCC, RCC_APB2Periph_GPIOx, etc.
 */
#include <ch32v00x.h>

/* -----------------------------------------------------------------------
 * PORT ALIASES
 * Shorthand for the three available GPIO ports.
 * These wrap the SDK's GPIOX pointers so your application code
 * never needs to include ch32v00x.h directly.
 * ---------------------------------------------------------------------- */
#define PORT_A    GPIOA
#define PORT_C    GPIOC
#define PORT_D    GPIOD

/* -----------------------------------------------------------------------
 * PIN MODE CONSTANTS
 *
 * These deliberately use a different naming pattern from the SDK's
 * GPIO_Mode_xxx enum to avoid any redefinition conflicts.
 *
 * The values are just integers — they are only used inside gpio_init()
 * in a switch statement, so there is no type collision with SDK enums.
 * ---------------------------------------------------------------------- */
#define GPIO_MODE_OUTPUT     (0x10U)   /* Push-pull output, 50 MHz         */
#define GPIO_MODE_INPUT      (0x11U)   /* Floating input, no pull resistor  */
#define GPIO_MODE_INPUT_PU   (0x12U)   /* Input with internal pull-UP       */
#define GPIO_MODE_INPUT_PD   (0x13U)   /* Input with internal pull-DOWN     */

/* -----------------------------------------------------------------------
 * PIN LEVEL CONSTANTS
 * ---------------------------------------------------------------------- */
#define GPIO_HIGH    (1U)
#define GPIO_LOW     (0U)

/* -----------------------------------------------------------------------
 * COMMON PIN SHORTCUTS
 * Default pin assignments for the VSDSquadron Mini board.
 * ---------------------------------------------------------------------- */
#define LED_PIN      (6U)    /* PD6 — onboard user LED (conflicts with UART RX) */
#define BTN_PIN      (4U)    /* PD4 — onboard push button (active-LOW)          */

/* -----------------------------------------------------------------------
 * FUNCTION PROTOTYPES
 * ---------------------------------------------------------------------- */

/*
 * gpio_init(port, pin, mode)
 *
 * Enables the port clock and configures the pin direction.
 * Must be called before any other gpio_* function on that pin.
 *
 * Parameters:
 *   port  — PORT_A, PORT_C, or PORT_D
 *   pin   — 0 through 7
 *   mode  — GPIO_MODE_OUTPUT / INPUT / INPUT_PU / INPUT_PD
 *
 * Example:
 *   gpio_init(PORT_C, 0, GPIO_MODE_OUTPUT);    // PC0 as LED
 *   gpio_init(PORT_D, BTN_PIN, GPIO_MODE_INPUT_PU); // PD4 button
 */
void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode);

/*
 * gpio_write(port, pin, level)
 *
 * Sets a configured output pin HIGH (3.3 V) or LOW (0 V).
 *
 * Parameters:
 *   level — GPIO_HIGH or GPIO_LOW
 *
 * Example:
 *   gpio_write(PORT_C, 0, GPIO_HIGH);   // LED on
 *   gpio_write(PORT_C, 0, GPIO_LOW);    // LED off
 */
void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t level);

/*
 * gpio_toggle(port, pin)
 *
 * Flips the current state of an output pin.
 * If HIGH → goes LOW, if LOW → goes HIGH.
 *
 * Example:
 *   gpio_toggle(PORT_C, 0);   // blink LED
 */
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);

/*
 * gpio_read(port, pin)
 *
 * Reads the live voltage on a pin.
 * Returns GPIO_HIGH (1) if the pin is at 3.3 V, GPIO_LOW (0) if at 0 V.
 * The pin must have been configured as an input mode first.
 *
 * Example:
 *   if (gpio_read(PORT_D, BTN_PIN) == GPIO_LOW) { // button pressed }
 */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);

#endif /* GPIO_H */