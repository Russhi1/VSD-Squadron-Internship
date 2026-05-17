#ifndef GPIO_H
#define GPIO_H

#include <ch32v00x.h>

#define PORT_A    GPIOA
#define PORT_C    GPIOC
#define PORT_D    GPIOD

#define GPIO_OUTPUT      0
#define GPIO_INPUT       1
#define GPIO_INPUT_PU    2
#define GPIO_INPUT_PD    3
#define GPIO_HIGH    1
#define GPIO_LOW     0

#define LED_PIN    6
#define BTN_PIN    4

void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode);
void gpio_set(GPIO_TypeDef *port, uint8_t pin);
void gpio_clear(GPIO_TypeDef *port, uint8_t pin);
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);

#endif