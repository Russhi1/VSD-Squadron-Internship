
#ifndef EVENTQ_H
#define EVENTQ_H

#include <stdint.h>

/* Maximum events buffered at once. Power of 2 keeps modulo fast. */
#define EVENTQ_SIZE    16

/* Maximum length of a UART command string stored in an event */
#define EVENTQ_CMD_LEN 24

typedef enum {
    PRIORITY_LOW    = 0,   /* routine housekeeping, e.g. timer ticks     */
    PRIORITY_NORMAL = 1,   /* user interaction, e.g. button presses      */
    PRIORITY_HIGH   = 2,   /* urgent, e.g. UART commands, sensor alerts  */
} EventPriority;


typedef enum {
    EVENT_NONE           = 0,
    EVENT_TIMER_TICK     = 1,   /* fires every 1000 ms                   */
    EVENT_BUTTON_PRESSED = 2,   /* confirmed debounced button press       */
    EVENT_UART_CMD       = 3,   /* '\n'-terminated command from terminal  */
} EventType;

typedef struct {
    EventType     type;
    EventPriority priority;
    uint32_t      timestamp;
    uint32_t      enqueue_time;
    char          cmd[EVENTQ_CMD_LEN];
} Event;

typedef struct {
    Event   buf[EVENTQ_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} EventQueue;

void eventq_init(EventQueue *q);

uint8_t eventq_push(EventQueue    *q,
                    EventType      type,
                    EventPriority  priority,
                    uint32_t       timestamp,
                    const char    *cmd);

uint8_t eventq_pop(EventQueue *q, Event *out);

/* Returns 1 if the queue contains no events, 0 otherwise. */
uint8_t eventq_is_empty(const EventQueue *q);

/* Returns the number of events currently waiting in the queue. */
uint8_t eventq_depth(const EventQueue *q);

#endif /* EVENTQ_H */