/*
 * eventq.h — Event Queue Library
 * VSD Squadron Mini (CH32V003F4U6)
 *
 * Circular FIFO queue with priority-aware dispatch.
 * Zero hardware dependencies — pure data structure.
 */

#ifndef EVENTQ_H
#define EVENTQ_H

#include <stdint.h>

/* Maximum events buffered at once. Power of 2 keeps modulo fast. */
#define EVENTQ_SIZE    16

/* Maximum length of a UART command string stored in an event */
#define EVENTQ_CMD_LEN 24

/* -----------------------------------------------------------------------
 * EventPriority — controls dispatch order when multiple events are queued.
 * Higher numeric value = dispatched first.
 * ---------------------------------------------------------------------- */
typedef enum {
    PRIORITY_LOW    = 0,   /* routine housekeeping, e.g. timer ticks     */
    PRIORITY_NORMAL = 1,   /* user interaction, e.g. button presses      */
    PRIORITY_HIGH   = 2,   /* urgent, e.g. UART commands, sensor alerts  */
} EventPriority;

/* -----------------------------------------------------------------------
 * EventType — one value per hardware or software event source.
 * ---------------------------------------------------------------------- */
typedef enum {
    EVENT_NONE           = 0,
    EVENT_TIMER_TICK     = 1,   /* fires every 1000 ms                   */
    EVENT_BUTTON_PRESSED = 2,   /* confirmed debounced button press       */
    EVENT_UART_CMD       = 3,   /* '\n'-terminated command from terminal  */
} EventType;

/* -----------------------------------------------------------------------
 * Event — the unit passed from producer to consumer.
 *
 * timestamp   : millis() when the event was produced
 * enqueue_time: millis() when eventq_push() was called
 *               (same as timestamp in most cases, but lets you measure
 *                queue latency: dispatch_time - enqueue_time)
 * cmd[]       : only populated for EVENT_UART_CMD; empty string otherwise
 * ---------------------------------------------------------------------- */
typedef struct {
    EventType     type;
    EventPriority priority;
    uint32_t      timestamp;
    uint32_t      enqueue_time;
    char          cmd[EVENTQ_CMD_LEN];
} Event;

/* -----------------------------------------------------------------------
 * EventQueue — the circular buffer.
 *
 * head  : index of oldest event (next to be popped)
 * tail  : index of next empty slot (next to be written)
 * count : number of valid events currently held
 * ---------------------------------------------------------------------- */
typedef struct {
    Event   buf[EVENTQ_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} EventQueue;

/* -----------------------------------------------------------------------
 * API
 * ---------------------------------------------------------------------- */

/*
 * eventq_init — reset queue to empty state.
 * Call once before any push or pop.
 */
void eventq_init(EventQueue *q);

/*
 * eventq_push — add a new event to the queue.
 *
 * Parameters:
 *   q         : pointer to the queue
 *   type      : event type (EVENT_TIMER_TICK, etc.)
 *   priority  : PRIORITY_LOW / NORMAL / HIGH
 *   timestamp : millis() at time of event production
 *   cmd       : command string for EVENT_UART_CMD, NULL for all others
 *
 * Returns 1 on success.
 * Returns 0 if queue was full — event is dropped, caller should count this.
 */
uint8_t eventq_push(EventQueue    *q,
                    EventType      type,
                    EventPriority  priority,
                    uint32_t       timestamp,
                    const char    *cmd);

/*
 * eventq_pop — remove and return the highest-priority event.
 *
 * Among events of equal priority, the oldest one (FIFO) is returned.
 *
 * Returns 1 if an event was written into *out.
 * Returns 0 if queue was empty — *out is untouched.
 */
uint8_t eventq_pop(EventQueue *q, Event *out);

/* Returns 1 if the queue contains no events, 0 otherwise. */
uint8_t eventq_is_empty(const EventQueue *q);

/* Returns the number of events currently waiting in the queue. */
uint8_t eventq_depth(const EventQueue *q);

#endif /* EVENTQ_H */