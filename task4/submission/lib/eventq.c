
#include "eventq.h"
#include <ch32v00x.h>   /* for __disable_irq / __enable_irq */



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

 
    *out    = q->buf[q->head];
    q->head = (uint8_t)((q->head + 1u) % EVENTQ_SIZE);

  
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