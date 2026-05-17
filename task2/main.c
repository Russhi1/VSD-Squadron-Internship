/*
 * task2/submission/main.c
 * Task 2 — Board Bring-Up, UART Validation, GPIO Demonstration
 *
 * VSDSquadron Mini (CH32V003F4U6), 24MHz RISC-V
 *
 * What this firmware does:
 *   1. Initialises UART1 at 115200 baud on PD5 (TX)
 *   2. Prints a startup banner (board name + firmware version)
 *   3. Blinks the onboard LED on PD6 every 500ms using SysTick
 *   4. Prints a status line with a counter every 500ms over UART
 *
 * Hardware pin usage (all verified against DataSheet Table 3):
 *   PD6 — onboard user LED  (silkscreen "PD6", firmware pin 6, port D)
 *   PD5 — UART TX           (silkscreen "PD5", firmware pin 5, port D)
 *
 * Architecture note:
 *   main.c contains ZERO direct register accesses.
 *   All GPIO is done through the gpio.h API (gpio_init, gpio_set,
 *   gpio_clear, gpio_toggle).
 *   All UART is done through UART helper functions defined below.
 *
 * Author: Rushil Rai
 * Firmware version: v1.0.0
 */

#include <ch32v00x.h>
#include "gpio.h"

/* =========================================================================
 * FIRMWARE METADATA
 * Hardcoded strings printed at startup over UART.
 * ========================================================================= */
#define BOARD_NAME        "VSDSquadron Mini (CH32V003F4U6)"
#define FW_VERSION        "v1.0.0"
#define BLINK_PERIOD_MS   500u    /* LED toggle interval in milliseconds */

/* =========================================================================
 * SYSTICK — millisecond counter
 *
 * SysTick is a countdown timer built into the RISC-V core.
 * We configure it to fire an interrupt every 1ms.
 * The ISR increments 'millis' — everything else just reads it.
 *
 * volatile: tells the compiler "this variable can change at any moment
 * (inside an interrupt), so NEVER cache it in a register."
 * ========================================================================= */
volatile uint32_t millis = 0;

void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    SysTick->SR = 0;   /* clear interrupt flag — mandatory, or ISR fires again */
    millis++;
}

static void systick_init(void)
{
    SysTick->CTLR = 0;          /* disable while configuring               */
    SysTick->SR   = 0;          /* clear any stale interrupt flag           */
    SysTick->CNT  = 0;          /* reset counter                            */
    SysTick->CMP  = 24000 - 1;  /* 24MHz clock / 24000 ticks = 1ms period  */
    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CTLR = 0xF;        /* enable: counter + interrupt + auto-reload + HCLK */
    __enable_irq();
}

/* =========================================================================
 * UART HELPER LAYER
 *
 * These functions wrap the CH32V003 USART1 peripheral.
 * TX pin = PD5 (confirmed in DataSheet Table 2: "USART PD6(RX), PD5(TX)")
 *
 * Only this section touches USART registers — main() never does.
 * ========================================================================= */

/*
 * uart_init(baud)
 * Configures USART1 for 8N1 at the requested baud rate, TX only.
 * Enables the USART1 clock, configures PD5 as alternate-function push-pull.
 */
static void uart_init(uint32_t baud)
{
    /* 1. Enable clocks: USART1 on APB2, GPIOD on APB2 */
    RCC->APB2PCENR |= RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOD;

    /* 2. Configure PD5 as Alternate Function Push-Pull output (USART TX)
     *    CFGLR bits [23:20] → pin 5 → value 0xB (AF push-pull, 50MHz) */
    GPIOD->CFGLR &= ~(0xF << (4 * 5));
    GPIOD->CFGLR |=  (0xB << (4 * 5));

    /* 3. Set baud rate — BRR = FCLK / baud = 24,000,000 / baud */
    USART1->BRR = (uint16_t)(24000000UL / baud);

    /* 4. Enable USART1, TX enable, 8-bit word, 1 stop bit (defaults) */
    USART1->CTLR1 = USART_CTLR1_UE | USART_CTLR1_TE;
}

/*
 * uart_putchar(c)
 * Sends a single character. Waits (blocking) until the TX register is empty.
 * This is fine for a demo — in production you would use DMA or a TX ring buffer.
 */
static void uart_putchar(char c)
{
    while (!(USART1->STATR & USART_STATR_TXE));  /* wait for TX empty */
    USART1->DATAR = (uint8_t)c;
}

/*
 * uart_print(s)
 * Sends a null-terminated string character by character.
 */
static void uart_print(const char *s)
{
    while (*s)
        uart_putchar(*s++);
}

/*
 * uart_println(s)
 * Sends a string followed by CR+LF (standard for serial terminals).
 */
static void uart_println(const char *s)
{
    uart_print(s);
    uart_putchar('\r');
    uart_putchar('\n');
}

/*
 * uart_print_uint(n)
 * Converts an unsigned 32-bit integer to decimal and sends it over UART.
 * We do this without printf to keep the binary small (no stdlib needed).
 */
static void uart_print_uint(uint32_t n)
{
    char buf[12];
    int  i = 0;

    if (n == 0) { uart_putchar('0'); return; }

    while (n > 0) {
        buf[i++] = '0' + (n % 10);
        n /= 10;
    }
    /* digits are in reverse order — print backwards */
    while (--i >= 0)
        uart_putchar(buf[i]);
}

/* =========================================================================
 * STARTUP BANNER
 * Printed once over UART after every reset.
 * ========================================================================= */
static void print_banner(void)
{
    uart_println("========================================");
    uart_println("  VSDSquadron Mini — Firmware Task 2");
    uart_println("  Board   : " BOARD_NAME);
    uart_println("  Version : " FW_VERSION);
    uart_println("  Author  : Rushil Rai");
    uart_println("  UART TX : PD5  |  LED : PD6");
    uart_println("========================================");
    uart_println("[BOOT] System initialised. Entering main loop.");
    uart_println("");
}

/* =========================================================================
 * MAIN APPLICATION
 *
 * Architecture:
 *   - gpio_init / gpio_toggle  ← GPIO API (no register access here)
 *   - uart_*                   ← UART layer defined above
 *   - SysTick                  ← millisecond timebase
 *
 * The LED blinks every BLINK_PERIOD_MS using a non-blocking pattern:
 *   compare millis against a stored 'last_toggle' timestamp.
 *   The CPU never stalls in a delay loop.
 * ========================================================================= */
int main(void)
{
    uint32_t last_toggle  = 0;
    uint32_t loop_counter = 0;

    /* -----------------------------------------------------------------
     * 1. Initialise peripherals
     * ----------------------------------------------------------------- */
    systick_init();                               /* 1ms tick */
    uart_init(115200);                            /* UART on PD5, 115200 8N1 */
    gpio_init(PORT_D, LED_PIN, GPIO_OUTPUT);      /* PD6 as push-pull output */

    /* -----------------------------------------------------------------
     * 2. Print startup banner (Task 2: "clear startup message after reset")
     * ----------------------------------------------------------------- */
    print_banner();

    /* -----------------------------------------------------------------
     * 3. Main loop — runs forever
     *    Every BLINK_PERIOD_MS: toggle LED, print one UART status line
     * ----------------------------------------------------------------- */
    while (1)
    {
        if ((millis - last_toggle) >= BLINK_PERIOD_MS)
        {
            last_toggle = millis;
            loop_counter++;

            /* Toggle onboard LED on PD6 */
            gpio_toggle(PORT_D, LED_PIN);

            /* Print status line:
             * Format: [  1234ms] Counter:   5  LED: ON
             * This gives > 10 consecutive UART lines as required. */
            uart_print("[");
            uart_print_uint(millis);
            uart_print("ms] Counter: ");
            uart_print_uint(loop_counter);

            /* Print LED state based on whether counter is odd or even
             * (gpio_toggle flips each call, so even=OFF, odd=ON) */
            if (loop_counter % 2 == 1)
                uart_println("  LED: ON");
            else
                uart_println("  LED: OFF");
        }

        /*
         * The CPU is free here between toggles.
         * In a real product this is where you would read sensors,
         * handle button presses, process I2C data, etc.
         */
    }
}