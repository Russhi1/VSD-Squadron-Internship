/*
 * eventq.c — Event queue implementation
 *
 * Circular buffer with head/tail/count.
 *
 * Memory layout (EVENTQ_SIZE = 8):
 *   Indices:  [0][1][2][3][4][5][6][7]
 *   After 3 pushes, 1 pop: head=1, tail=3, count=2
 *   Valid events sit at indices [1] and [2].
 *
 * Wrap-around: index = (index + 1) % EVENTQ_SIZE.
 * Because EVENTQ_SIZE is a power of 2 the compiler will optimise this
 * to a bitwise AND, but the modulo form is written for readability.
 *
 * ISR safety:
 *   push() is called from the EXTI ISR; it disables IRQs for the three
 *   lines that modify shared state (buf[tail], tail, count), then
 *   re-enables them.  The window is deterministically short (~6 cycles).
 *   pop() only runs in main-loop context; it only modifies head and count.
 *   Because head is never touched by the ISR, pop() needs no protection.
 */

#include "eventq.h"
#include <ch32v00x.h>   /* for __disable_irq / __enable_irq */

/* ── Public API ─────────────────────────────────────────────────────── */

void eventq_init(EventQueue *q)
{
    q->head  = 0u;
    q->tail  = 0u;
    q->count = 0u;
}

uint8_t eventq_push(EventQueue *q, EventType type, uint32_t ts)
{
    /* Check capacity before entering critical section */
    if (q->count >= EVENTQ_SIZE) {
        return 0u;   /* Full — drop the event */
    }

    /*
     * Critical section: disable interrupts while writing to shared
     * state.  This prevents a nested ISR from corrupting tail/count.
     */
    __disable_irq();

    q->buf[q->tail].type      = type;
    q->buf[q->tail].timestamp = ts;
    q->tail  = (uint8_t)((q->tail + 1u) % EVENTQ_SIZE);
    q->count++;

    __enable_irq();
    return 1u;
}

uint8_t eventq_pop(EventQueue *q, Event *out)
{
    if (q->count == 0u) {
        return 0u;   /* Empty */
    }

    /*
     * Copy the event out.  head is only written here (main-loop
     * context) so no critical section is required for the read.
     * count is decremented after the copy so the slot is not
     * reclaimed until we are finished reading it.
     */
    *out    = q->buf[q->head];
    q->head = (uint8_t)((q->head + 1u) % EVENTQ_SIZE);

    /*
     * Decrement count with a short critical section because the ISR
     * may be concurrently incrementing it.
     */
    __disable_irq();
    q->count--;
    __enable_irq();

    return 1u;
}

uint8_t eventq_depth(const EventQueue *q)
{
    return q->count;
}

uint8_t eventq_empty(const EventQueue *q)
{
    return (q->count == 0u) ? 1u : 0u;
}

const char *event_name(EventType t)
{
    switch (t) {
        case EVT_SHORT_PRESS:  return "SHORT_PRESS";
        case EVT_LONG_PRESS:   return "LONG_PRESS ";
        case EVT_DOUBLE_TAP:   return "DOUBLE_TAP ";
        default:               return "UNKNOWN    ";
    }
}