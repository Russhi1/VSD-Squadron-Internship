/* timer.h */
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>

/*
 * Configure SysTick to fire every 1ms at 24MHz.
 * Must be called once before timer_get_millis().
 */
void     timer_init(void);

/*
 * Returns milliseconds elapsed since timer_init().
 * Updated atomically by the SysTick ISR.
 */
uint32_t timer_get_millis(void);

#endif /* TIMER_H */