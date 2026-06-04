/*
 * eventq.c — Event Queue Implementation
 *
 * Push appends to the tail (O(1)).
 * Pop finds the highest-priority event anywhere in the buffer (O(n)),
 * then removes it by shifting the gap closed.
 * n is at most EVENTQ_SIZE (16), so O(n) here is negligible.
 */

#include "eventq.h"
#include <string.h>    /* strncpy, memset */

void eventq_init(EventQueue *q)
{
    q->head  = 0;
    q->tail  = 0;
    q->count = 0;
}

uint8_t eventq_push(EventQueue    *q,
                    EventType      type,
                    EventPriority  priority,
                    uint32_t       timestamp,
                    const char    *cmd)
{
    /* Drop the event if the buffer is full */
    if (q->count >= EVENTQ_SIZE) {
        return 0;
    }

    Event *slot        = &q->buf[q->tail];
    slot->type         = type;
    slot->priority     = priority;
    slot->timestamp    = timestamp;
    slot->enqueue_time = timestamp;   /* caller passes millis() directly */

    if (cmd != (void *)0) {
        strncpy(slot->cmd, cmd, EVENTQ_CMD_LEN - 1);
        slot->cmd[EVENTQ_CMD_LEN - 1] = '\0';
    } else {
        slot->cmd[0] = '\0';
    }

    /* Advance tail with wrap-around */
    q->tail = (uint8_t)((q->tail + 1u) % EVENTQ_SIZE);
    q->count++;
    return 1;
}

uint8_t eventq_pop(EventQueue *q, Event *out)
{
    if (q->count == 0) {
        return 0;
    }

    /*
     * Scan all valid entries to find the highest-priority event.
     * On a tie, the entry closest to head wins (oldest first = FIFO).
     */
    uint8_t best_slot = q->head;
    uint8_t i;

    for (i = 1u; i < q->count; i++) {
        uint8_t idx = (uint8_t)((q->head + i) % EVENTQ_SIZE);
        if (q->buf[idx].priority > q->buf[best_slot].priority) {
            best_slot = idx;
        }
    }

    /* Copy the winning event to the caller */
    *out = q->buf[best_slot];

    /*
     * Close the gap left by removing best_slot.
     * Shift every entry between best_slot and tail one step toward head.
     */
    i = best_slot;
    while (i != q->tail) {
        uint8_t next   = (uint8_t)((i + 1u) % EVENTQ_SIZE);
        q->buf[i]      = q->buf[next];
        i              = next;
    }

    /* Retract tail by one */
    q->tail = (uint8_t)((q->tail + EVENTQ_SIZE - 1u) % EVENTQ_SIZE);
    q->count--;
    return 1;
}

uint8_t eventq_is_empty(const EventQueue *q)
{
    return (q->count == 0u) ? 1u : 0u;
}

uint8_t eventq_depth(const EventQueue *q)
{
    return q->count;
}