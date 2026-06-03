/*
 * timer.h — SysTick 1 ms counter for CH32V003
 *
 * Provides a monotonic millisecond clock derived from the SysTick
 * peripheral running at HCLK (24 MHz).  The ISR fires every 1 ms and
 * increments an internal volatile counter.
 *
 * The counter wraps to zero after ~49.7 days; all callers must use
 * unsigned subtraction (now - start) to handle wrap-around correctly.
 */

#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/**
 * @brief  Initialise SysTick for a 1 ms interrupt period.
 *
 * Configures SysTick->CMP = 24000 - 1 (24 MHz / 1000 Hz − 1),
 * registers the VTF fast-interrupt handler, and calls __enable_irq().
 * Must be called once before timer_get_millis().
 */
void timer_init(void);

/**
 * @brief  Return the number of milliseconds since timer_init() was called.
 * @return Monotonic uint32_t millisecond count.  Wraps at ~49.7 days.
 *
 * The underlying counter is declared volatile; no critical section is
 * needed on this single-core device for a 32-bit aligned read.
 */
uint32_t timer_get_millis(void);

#endif /* TIMER_H */