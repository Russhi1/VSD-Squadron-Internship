#include <ch32v00x.h>
#include "gpio.h"
#include "uart.h"

#define BOARD_NAME       "VSDSquadron Mini (CH32V003F4U6)"
#define FW_VERSION       "v1.0.0"
#define BLINK_PERIOD_MS   500u

volatile uint32_t millis = 0;

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    SysTick->SR = 0;
    millis++;
}

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

static void print_banner(void)
{
    uart_println("========================================");
    uart_println("  VSDSquadron Mini - Firmware Task 2");
    uart_println("  Board   : " BOARD_NAME);
    uart_println("  Version : " FW_VERSION);
    uart_println("  Author  : Rushil Rai");
    uart_println("  UART TX : PD5  |  LED : PD6");
    uart_println("========================================");
    uart_println("[BOOT] System initialised.");
    uart_println("");
}

int main(void)
{
    uint32_t last_toggle  = 0;
    uint32_t loop_counter = 0;

    systick_init();
    uart_init(115200);
    gpio_init(PORT_D, LED_PIN, GPIO_OUTPUT);

    print_banner();

    while (1)
    {
        if ((millis - last_toggle) >= BLINK_PERIOD_MS)
        {
            last_toggle = millis;
            loop_counter++;

            gpio_toggle(PORT_D, LED_PIN);

            uart_print("[");
            uart_print_uint(millis);
            uart_print("ms] Counter: ");
            uart_print_uint(loop_counter);

            if (loop_counter % 2 == 1)
                uart_println("  LED: ON");
            else
                uart_println("  LED: OFF");
        }
    }
}