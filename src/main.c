#include <ch32v00x.h>
#include "gpio.h"
#include "timer.h"
#include "uart.h"

/* -----------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------- */
int main(void)
{
    /* Initialise peripherals */
    timer_init();
    uart_init(115200);
    gpio_init(PORT_D, LED_PIN, GPIO_OUTPUT);
    gpio_init(PORT_D, BTN_PIN, GPIO_INPUT_PU);
    gpio_write(PORT_D, LED_PIN, GPIO_LOW);   /* LED starts OFF */

    /* Startup banner */
    uart_println("=========================================");
    uart_println("  Advanced GPIO Library - Press Button Demo");
    uart_println("  VSDSquadron Mini | CH32V003F4U6");
    uart_println("=========================================");
    uart_println("[INIT] SysTick  : 1 ms tick active");
    uart_println("[INIT] UART     : PD5, 115200 baud, 8N1");
    uart_println("[INIT] LED      : PD6, output, starts OFF");
    uart_println("[INIT] Button   : PD4, pull-up, active LOW");
    uart_println("[INIT] Debounce : 5 samples x ~40 us each");
    uart_println("-----------------------------------------");
    uart_println("[INFO] Press the button to toggle the LED");
    uart_println("-----------------------------------------");

    /* State tracking */
    uint8_t  prev_btn    = GPIO_HIGH;
    uint8_t  led_state   = GPIO_LOW;
    uint32_t press_count = 0;
    uint32_t press_time  = 0;

    while (1)
    {
        uint8_t btn = gpio_debounce_read(PORT_D, BTN_PIN, 5);

        if (btn == GPIO_DEBOUNCE_UNSTABLE) continue;

        if (btn == GPIO_LOW && prev_btn == GPIO_HIGH)
        {
            press_count++;
            press_time = timer_get_millis();

            led_state = (led_state == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH;
            gpio_write(PORT_D, LED_PIN, led_state);

            uart_print("[PRESS]   #");
            uart_print_num(press_count);
            uart_print("  |  t = ");
            uart_print_num(timer_get_millis());
            uart_print(" ms  |  LED = ");
            uart_println(led_state ? "ON " : "OFF");
        }

        if (btn == GPIO_HIGH && prev_btn == GPIO_LOW)
        {
            uint32_t release_time = timer_get_millis();

            uart_print("[RELEASE]     |  t = ");
            uart_print_num(release_time);
            uart_print(" ms  |  held = ");
            uart_print_num(release_time - press_time);
            uart_println(" ms");
        }

        prev_btn = btn;
    }
}