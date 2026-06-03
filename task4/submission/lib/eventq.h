

#ifndef EVENTQ_H
#define EVENTQ_H

#include <stdint.h>

/* ── Queue capacity ─────────────────────────────────────────────────── */
#define EVENTQ_SIZE   8u   /* Must be a power of 2; 8 events is plenty  */

/* ── Event types ────────────────────────────────────────────────────── */
typedef enum {
    EVT_NONE         = 0,
    EVT_SHORT_PRESS  = 1,   /* Button released in < 500 ms              */
    EVT_LONG_PRESS   = 2,   /* Button held for >= 500 ms then released  */
    EVT_DOUBLE_TAP   = 3,   /* Two short presses within 400 ms          */
} EventType;

/* ── Event record ───────────────────────────────────────────────────── */
typedef struct {
    EventType type;       /* What happened                              */
    uint32_t  timestamp;  /* timer_get_millis() at moment of push       */
} Event;

/* ── Queue object ───────────────────────────────────────────────────── */
typedef struct {
    Event   buf[EVENTQ_SIZE];
    uint8_t head;    /* Next slot to read  (pop side)                   */
    uint8_t tail;    /* Next slot to write (push side)                  */
    uint8_t count;   /* Current number of events in buffer              */
} EventQueue;

/* ── Public API ─────────────────────────────────────────────────────── */

/**
 * @brief  Initialise the queue.  Call once before any push/pop.
 * @param  q  Pointer to an EventQueue allocated by the caller.
 */
void eventq_init(EventQueue *q);

/**
 * @brief  Push one event onto the tail of the queue.
 * @param  q    The queue.
 * @param  type Event type tag.
 * @param  ts   Timestamp in ms (from timer_get_millis()).
 * @return 1 on success, 0 if the queue was full (event dropped).
 *
 * Safe to call from an ISR — wraps the modification in a critical
 * section using __disable_irq() / __enable_irq().
 */
uint8_t eventq_push(EventQueue *q, EventType type, uint32_t ts);

/**
 * @brief  Pop the oldest event from the head of the queue.
 * @param  q    The queue.
 * @param  out  Destination; filled only when return value is 1.
 * @return 1 if an event was dequeued, 0 if the queue was empty.
 *
 * Called from main-loop context only; no critical section needed for
 * the pop itself because head is only written here.
 */
uint8_t eventq_pop(EventQueue *q, Event *out);

/**
 * @brief  Return the number of events currently in the queue.
 */
uint8_t eventq_depth(const EventQueue *q);

/**
 * @brief  Return 1 if the queue is empty, 0 otherwise.
 */
uint8_t eventq_empty(const EventQueue *q);

/* ── Human-readable event name (for UART logging) ───────────────────── */

/**
 * @brief  Return a short constant string name for an event type.
 * @param  t  EventType value.
 * @return Pointer to a string literal; never NULL.
 */
const char *event_name(EventType t);

#endif /* EVENTQ_H */