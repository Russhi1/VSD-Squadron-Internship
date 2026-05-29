#include <stdio.h>
#include "gpio.h"


void gpio_init(int pin, int direction)
{
    if (direction == GPIO_OUTPUT) {
        printf("GPIO %d initialized as OUTPUT\n", pin);
    } else {
        printf("GPIO %d initialized as INPUT\n", pin);
    }
}

void gpio_write(int pin, int value)
{
    printf("GPIO %d write value: %d\n", pin, value);
}

int gpio_read(int pin)
{
    printf("GPIO %d read value\n", pin);
    return 1; // simulated value
}
