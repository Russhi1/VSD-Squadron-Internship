

#include <ch32v00x.h>
#include "gpio.h"
#include "uart.h"

/* -----------------------------------------------------------------------
 * SysTick — 1 ms time base
 * CMP = 24 000 - 1 gives exactly 1 ms at 24 MHz.
 * volatile: variable can change inside the ISR at any time.
 * ---------------------------------------------------------------------- */
volatile uint32_t millis = 0;

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void) { SysTick->SR = 0; millis++; }

static void systick_init(void)
{
    SysTick->CTLR = 0;
    SysTick->SR   = 0;
    SysTick->CNT  = 0;
    SysTick->CMP  = 24000 - 1;
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CTLR = 0xF;
    __enable_irq();
}

/* -----------------------------------------------------------------------
 * Main
 * ---------------------------------------------------------------------- */
int main(void)
{
    /* Initialise peripherals */
    systick_init();
    uart_init(115200);
    gpio_init(PORT_D, LED_PIN, GPIO_OUTPUT);
    gpio_init(PORT_D, BTN_PIN, GPIO_INPUT_PU);
    gpio_write(PORT_D, LED_PIN, GPIO_LOW);   /* LED starts OFF */

    /* Startup banner */
    uart_println("=========================================");
    uart_println("  Advanced GPIO Library — Press Button Demo");
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
    uint8_t  prev_btn    = GPIO_HIGH;   /* button starts released */
    uint8_t  led_state   = GPIO_LOW;    /* LED starts OFF         */
    uint32_t press_count = 0;
    uint32_t press_time  = 0;

    while (1)
    {
        /*
         * gpio_debounce_read() takes 5 samples with ~40 µs between each.
         * Returns GPIO_HIGH / GPIO_LOW when stable, or
         * GPIO_DEBOUNCE_UNSTABLE (0xFF) when the pin is still bouncing.
         * Unstable readings are ignored so one press = one event.
         */
        uint8_t btn = gpio_debounce_read(PORT_D, BTN_PIN, 5);

        if (btn == GPIO_DEBOUNCE_UNSTABLE) continue;

        /* Falling edge — button pressed (HIGH → LOW) */
        if (btn == GPIO_LOW && prev_btn == GPIO_HIGH)
        {
            press_count++;
            press_time = millis;

            /* Toggle LED using gpio_write */
            led_state = (led_state == GPIO_HIGH) ? GPIO_LOW : GPIO_HIGH;
            gpio_write(PORT_D, LED_PIN, led_state);

            uart_print("[PRESS]   #");
            uart_print_num(press_count);
            uart_print("  |  t = ");
            uart_print_num(millis);
            uart_print(" ms  |  LED = ");
            uart_println(led_state ? "ON " : "OFF");
        }

        /* Rising edge — button released (LOW → HIGH) */
        if (btn == GPIO_HIGH && prev_btn == GPIO_LOW)
        {
            uart_print("[RELEASE]     |  t = ");
            uart_print_num(millis);
            uart_print(" ms  |  held = ");
            uart_print_num(millis - press_time);
            uart_println(" ms");
        }

        prev_btn = btn;
    }
}