
#ifndef GPIO_H
#define GPIO_H

#include <ch32v00x.h>

/* Port aliases */
#define PORT_A  GPIOA
#define PORT_C  GPIOC
#define PORT_D  GPIOD

/* Pin modes */
#define GPIO_OUTPUT    0   /* Push-pull output, 50 MHz */
#define GPIO_INPUT     1   /* Floating input, no pull  */
#define GPIO_INPUT_PU  2   /* Input with pull-up       */
#define GPIO_INPUT_PD  3   /* Input with pull-down     */

/* Logic levels */
#define GPIO_HIGH  1
#define GPIO_LOW   0

/* Returned by gpio_debounce_read() when pin is still bouncing */
#define GPIO_DEBOUNCE_UNSTABLE  0xFF

/* Onboard peripherals */
#define LED_PIN  6   /* PD6 — onboard LED    */
#define BTN_PIN  4   /* PD4 — onboard button */

/* API */
void    gpio_init          (GPIO_TypeDef *port, uint8_t pin, uint8_t mode);
void    gpio_write         (GPIO_TypeDef *port, uint8_t pin, uint8_t value);
void    gpio_toggle        (GPIO_TypeDef *port, uint8_t pin);
uint8_t gpio_read          (GPIO_TypeDef *port, uint8_t pin);
uint8_t gpio_debounce_read (GPIO_TypeDef *port, uint8_t pin, uint8_t samples);

#endif /* GPIO_H */