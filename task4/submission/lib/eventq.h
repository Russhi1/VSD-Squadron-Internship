
#ifndef EVENTQ_H
#define EVENTQ_H

#include <stdint.h>

/* Maximum events buffered at once. Power of 2 keeps modulo fast. */
#define EVENTQ_SIZE    16

/* Maximum length of a UART command string stored in an event */
#define EVENTQ_CMD_LEN 24

typedef enum {
    EVENT_NONE = 0,
    EVENT_TIMER_TICK,        /* fired every 1000 ms by the timer producer   */
    EVENT_BUTTON_PRESSED,    /* fired on confirmed button press (debounced)  */
    EVENT_UART_CMD,          /* fired when a '\n'-terminated command arrives */
} EventType;


typedef struct {
    EventType type;
    uint32_t  timestamp;
    char      cmd[EVENTQ_CMD_LEN];
} Event;

typedef struct {
    Event   buf[EVENTQ_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} EventQueue;

void    eventq_init(EventQueue *q);


uint8_t eventq_push(EventQueue *q,
                    EventType   type,
                    uint32_t    timestamp,
                    const char *cmd);

uint8_t eventq_pop(EventQueue *q, Event *out);

/* Returns 1 if the queue has no events, 0 otherwise. */
uint8_t eventq_is_empty(const EventQueue *q);

/* Returns current number of events waiting in queue. */
uint8_t eventq_depth(const EventQueue *q);

#endif /* EVENTQ_H */