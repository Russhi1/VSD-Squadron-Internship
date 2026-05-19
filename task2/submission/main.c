#include "gpio.h"
#include "uart.h"

int main(void)
{
    systick_init();
    uart_init(115200);
    gpio_init(PORT_D, LED_PIN, GPIO_OUTPUT);

    uart_println("VSDSquadron Mini - Task 2");

    while (1)
    {
        if ((millis - last_toggle) >= 500)
        {
            last_toggle = millis;
            loop_counter++;
            gpio_toggle(PORT_D, LED_PIN);
            uart_print("[");
            uart_print_uint(millis);
            uart_println("ms] LED toggled");
        }
    }
}