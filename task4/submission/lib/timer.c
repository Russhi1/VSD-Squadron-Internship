/* timer.c */
#include "timer.h"
#include <ch32v00x.h>

/*
 * volatile: tells the compiler this variable can change at any time
 * (from the ISR), so it must be re-read from memory on every access
 * rather than cached in a register.
 */
volatile uint32_t _millis_count = 0;

/* SysTick ISR — runs automatically every 1ms */
void SysTick_Handler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void SysTick_Handler(void)
{
    SysTick->SR = 0;       /* clear the compare-match flag or it keeps firing */
    _millis_count++;
}

void timer_init(void)
{
    SysTick->CTLR = 0;           /* disable while configuring              */
    SysTick->SR   = 0;           /* clear any pending interrupt            */
    SysTick->CNT  = 0;           /* reset counter to 0                     */
    SysTick->CMP  = 24000 - 1;   /* 24,000 clocks at 24MHz = exactly 1ms   */

    NVIC_EnableIRQ(SysTicK_IRQn);

    /* CTLR = 0xF: enable counter | enable interrupt | auto-reload | HCLK */
    SysTick->CTLR = 0xF;

    __enable_irq();
}

uint32_t timer_get_millis(void)
{
    return _millis_count;
}