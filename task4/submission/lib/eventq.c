/*
 * eventq.c — Event Queue Implementation
 *
 * Circular buffer with head/tail/count tracking.
 *
 * Why circular buffer?
 *   - O(1) push and pop: no shifting of elements
 *   - Fixed memory: no dynamic allocation (malloc is banned in bare-metal)
 *   - Predictable worst-case timing: essential for embedded systems
 *
 * Memory layout example (EVENTQ_SIZE = 4):
 *
 *   Index:  [0]  [1]  [2]  [3]
 *   After 3 pushes, 1 pop:
 *     head = 1, tail = 3, count = 2
 *     Valid events are at [1] and [2]
 */

#include "eventq.h"
#include <string.h>   /* strncpy */

void eventq_init(EventQueue *q)
{
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}

uint8_t eventq_push(EventQueue *q,
                    EventType   type,
                    uint32_t    timestamp,
                    const char *cmd)
{
    /* Reject if full — caller decides how to handle the overflow */
    if (q->count >= EVENTQ_SIZE) {
        return 0;
    }

    Event *slot = &q->buf[q->tail];
    slot->type      = type;
    slot->timestamp = timestamp;

    if (cmd != (void *)0) {
        strncpy(slot->cmd, cmd, EVENTQ_CMD_LEN - 1);
        slot->cmd[EVENTQ_CMD_LEN - 1] = '\0';   /* always null-terminate */
    } else {
        slot->cmd[0] = '\0';
    }

    /* Advance tail with wrap-around */
    q->tail = (uint8_t)((q->tail + 1) % EVENTQ_SIZE);
    q->count++;
    return 1;
}

uint8_t eventq_pop(EventQueue *q, Event *out)
{
    if (q->count == 0) {
        return 0;   /* nothing to pop */
    }

    /* Copy the event out to the caller */
    *out = q->buf[q->head];

    /* Advance head with wrap-around */
    q->head = (uint8_t)((q->head + 1) % EVENTQ_SIZE);
    q->count--;
    return 1;
}

uint8_t eventq_is_empty(const EventQueue *q)
{
    return (q->count == 0) ? 1 : 0;
}

uint8_t eventq_depth(const EventQueue *q)
{
    return q->count;
}