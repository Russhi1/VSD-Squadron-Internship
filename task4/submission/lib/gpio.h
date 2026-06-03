

#ifndef GPIO_H
#define GPIO_H

#include <stdint.h>

/* ── Port identifiers ──────────────────────────────────────────────── */
#define PORT_A  0
#define PORT_C  1
#define PORT_D  2


#define GPIO_MODE_OUTPUT      0x03u  /* Push-pull output, 30 MHz max     */
#define GPIO_MODE_INPUT_FLOAT 0x04u  /* Floating input  (CNF=01,MODE=00) */
#define GPIO_MODE_INPUT_PU    0x08u  /* Pull-up input   (CNF=10,MODE=00) */

/* ── Logic level aliases ───────────────────────────────────────────── */
#define GPIO_HIGH  1u
#define GPIO_LOW   0u

/* ── Board-specific pin assignments ───────────────────────────────── */
#define LED_PORT   PORT_C   /* Onboard / external LED on PC0            */
#define LED_PIN    0

#define BTN_PORT   PORT_D   /* Onboard push-button on PD4, active-LOW   */
#define BTN_PIN    4

/* ── Public API ────────────────────────────────────────────────────── */

/**
 * @brief  Configure a single GPIO pin direction and mode.
 * @param  port  PORT_A / PORT_C / PORT_D
 * @param  pin   Pin number 0–7
 * @param  mode  GPIO_MODE_OUTPUT | GPIO_MODE_INPUT_FLOAT | GPIO_MODE_INPUT_PU
 *
 * The RCC clock for the target port is enabled inside this function.
 */
void gpio_init(uint8_t port, uint8_t pin, uint8_t mode);

/**
 * @brief  Drive an output pin high or low using atomic BSR/BCR registers.
 * @param  port   PORT_A / PORT_C / PORT_D
 * @param  pin    Pin number 0–7
 * @param  value  GPIO_HIGH or GPIO_LOW
 *
 * Uses GPIOx_BSHR (set) and GPIOx_BCR (reset) — no read-modify-write.
 * Safe to call from an ISR.
 */
void gpio_write(uint8_t port, uint8_t pin, uint8_t value);

/**
 * @brief  Read the current logic level of a pin from GPIOx_INDR.
 * @param  port  PORT_A / PORT_C / PORT_D
 * @param  pin   Pin number 0–7
 * @return GPIO_HIGH (1) or GPIO_LOW (0)
 */
uint8_t gpio_read(uint8_t port, uint8_t pin);

#endif /* GPIO_H */