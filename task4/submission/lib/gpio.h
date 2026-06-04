

#ifndef GPIO_H
#define GPIO_H

#include <ch32v00x.h>

#define PORT_A    GPIOA
#define PORT_C    GPIOC
#define PORT_D    GPIOD
#define GPIO_MODE_OUTPUT     (0x10U)   /* Push-pull output, 50 MHz         */
#define GPIO_MODE_INPUT      (0x11U)   /* Floating input, no pull resistor  */
#define GPIO_MODE_INPUT_PU   (0x12U)   /* Input with internal pull-UP       */
#define GPIO_MODE_INPUT_PD   (0x13U)   /* Input with internal pull-DOWN     */


#define GPIO_HIGH    (1U)
#define GPIO_LOW     (0U)

#define LED_PIN      (6U)    /* PD6 — onboard user LED (conflicts with UART RX) */
#define BTN_PIN      (4U)    /* PD4 — onboard push button (active-LOW)          */


void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode);

void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t level);

void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);

uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);

#endif /* GPIO_H */