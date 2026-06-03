
#include "timer.h"
#include <ch32v00x.h>

/* Volatile: modified inside SysTick_Handler (interrupt context) */
volatile uint32_t _millis = 0u;

/* ── ISR ────────────────────────────────────────────────────────────── */
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    SysTick->SR = 0u;   /* Clear CNTIF; must be written 0, not 1 */
    _millis++;
}

/* ── Public API ─────────────────────────────────────────────────────── */
void timer_init(void)
{
    SysTick->CTLR = 0u;           /* Disable while configuring           */
    SysTick->SR   = 0u;           /* Clear any pending flag              */
    SysTick->CNT  = 0u;           /* Reset counter                       */
    SysTick->CMP  = 24000u - 1u;  /* 24 MHz / 24000 = 1000 Hz = 1 ms    */

    NVIC_EnableIRQ(SysTicK_IRQn);
    SysTick->CTLR = 0x0Fu;

    __enable_irq();
}

uint32_t timer_get_millis(void)
{
    /*
     * On CH32V003 (single-core, no out-of-order execution) a 32-bit
     * aligned RAM read is atomic with respect to an 8-bit ISR write.
     * No critical section is required.
     */
    return _millis;
}