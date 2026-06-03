/*
 * timer.c — SysTick 1 ms counter implementation for CH32V003
 *
 * Register map (CH32V003 Reference Manual v1.9, Section 6.5.4):
 *   STK_CTLR  0xE000F000  — control: STE | STIE | STCLK | STRE
 *   STK_SR    0xE000F004  — status:  CNTIF (write 0 to clear)
 *   STK_CNTL  0xE000F008  — 32-bit up-counter
 *   STK_CMPLR 0xE000F010  — compare value; CNTIF set when CNT == CMP
 *
 * CTLR bits used:
 *   bit 0  STE   — start/enable counter
 *   bit 1  STIE  — enable compare-match interrupt
 *   bit 2  STCLK — 1 = HCLK source (24 MHz), 0 = HCLK/8
 *   bit 3  STRE  — auto-reload (count resets to 0 on match)
 *
 * The ISR is registered with WCH-Interrupt-fast which uses the PFIC's
 * VTF (Vector Table Free) path, cutting interrupt latency to ~2 cycles.
 */

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

    /*
     * CTLR = 0xF:
     *   STE=1   enable counter
     *   STIE=1  enable interrupt on match
     *   STCLK=1 use HCLK (24 MHz)
     *   STRE=1  auto-reload counter to 0 on match
     */
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