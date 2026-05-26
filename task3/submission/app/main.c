
#include <ch32v00x.h>
#include "gpio.h"
#include "uart.h"

/* 1 ms time base via SysTick */
volatile uint32_t millis = 0;
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void) { SysTick->SR = 0; millis++; }

static void systick_init(void)
{
    SysTick->CTLR = 0; SysTick->SR = 0; SysTick->CNT = 0;
    SysTick->CMP  = 24000 - 1;
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CTLR = 0xF;
    __enable_irq();
}

static void delay_ms(uint32_t ms)
{
    uint32_t start = millis;
    while ((millis - start) < ms);
}

int main(void)
{
    systick_init();
    uart_init(115200);
    gpio_init(PORT_D, LED_PIN, GPIO_OUTPUT);
    gpio_clear(PORT_D, LED_PIN);

    uart_println("========================================");
    uart_println("  Advanced GPIO Library Demo");
    uart_println("  VSDSquadron Mini | CH32V003F4U6");
    uart_println("========================================");
    uart_println("[INIT] SysTick : 1 ms tick active");
    uart_println("[INIT] UART    : PD5, 115200 baud, 8N1");
    uart_println("[INIT] LED     : PD6, output, starts OFF");
    uart_println("----------------------------------------");

    /* --- Part 1: GPIO Read API test (no wires needed) --- */
    uart_println("\n--- Part 1: GPIO Read Test (Wire-Free) ---");

    gpio_init(PORT_C, 4, GPIO_INPUT_PU);
    delay_ms(10);
    uart_print("[READ] PC4 pull-up   -> ");
    uart_println(gpio_read(PORT_C, 4) == GPIO_HIGH ? "HIGH [PASS]" : "LOW  [FAIL]");

    gpio_init(PORT_C, 4, GPIO_INPUT_PD);
    delay_ms(10);
    uart_print("[READ] PC4 pull-down -> ");
    uart_println(gpio_read(PORT_C, 4) == GPIO_LOW ? "LOW  [PASS]" : "HIGH [FAIL]");

    uart_println("--- Part 1 complete ---");

    /* --- Part 2: LED pattern loop --- */
    uart_println("\n--- Part 2: LED Pattern Demo ---");

    uint32_t cycle = 1;

    while (1)
    {
        uart_print("\n=== Cycle "); uart_print_num(cycle);
        uart_print(" | t = ");     uart_print_num(millis);
        uart_println(" ms ===");

        /* Phase 1 — rapid strobe (gpio_toggle) */
        uart_println("[P1] Rapid Strobe (gpio_toggle x6)");
        for (int i = 0; i < 6; i++) { gpio_toggle(PORT_D, LED_PIN); delay_ms(100); }
        gpio_clear(PORT_D, LED_PIN);
        delay_ms(500);

        /* Phase 2 — SOS (gpio_set / gpio_clear) */
        uart_println("[P2] SOS Pattern (gpio_set / gpio_clear)");

        uart_println("  S: short short short");
        for (int i = 0; i < 3; i++)
        { gpio_set(PORT_D, LED_PIN); delay_ms(150); gpio_clear(PORT_D, LED_PIN); delay_ms(150); }
        delay_ms(300);

        uart_println("  O: long  long  long");
        for (int i = 0; i < 3; i++)
        { gpio_set(PORT_D, LED_PIN); delay_ms(600); gpio_clear(PORT_D, LED_PIN); delay_ms(150); }
        delay_ms(300);

        uart_println("  S: short short short");
        for (int i = 0; i < 3; i++)
        { gpio_set(PORT_D, LED_PIN); delay_ms(150); gpio_clear(PORT_D, LED_PIN); delay_ms(150); }

        uart_print("[P2] Done | t = "); uart_print_num(millis); uart_println(" ms");
        uart_println("Waiting 2s...");
        delay_ms(2000);
        cycle++;
    }
}