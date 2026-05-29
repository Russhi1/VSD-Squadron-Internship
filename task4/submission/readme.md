# Project-9: Event Queue Framework + Demo Application

## Description

An event-driven firmware framework for the VSDSquadron Mini (CH32V003F4U6).
The system implements a circular event queue that decouples hardware event
detection (producers) from application logic (consumers). Three hardware
sources — a millisecond timer, a debounced push button, and UART command
input — independently push typed events into the queue. A central dispatcher
routes each event to the correct handler. No handler ever polls hardware
directly, and no producer knows what will happen to its events.

## Drivers Used

| Driver   | Role                                    |
|----------|-----------------------------------------|
| `gpio`   | LED output (PC0), Button input (PD4)    |
| `uart`   | Serial terminal TX=PD5, RX=PD6          |
| `timer`  | SysTick 1ms tick, used by timer producer|
| `eventq` | Circular FIFO queue                     |

## API Summary

### eventq library
```c
void    eventq_init(EventQueue *q);
uint8_t eventq_push(EventQueue *q, EventType type, uint32_t ts, const char *cmd);
uint8_t eventq_pop(EventQueue *q, Event *out);
uint8_t eventq_is_empty(const EventQueue *q);
uint8_t eventq_depth(const EventQueue *q);
```

### timer library
```c
void     timer_init(void);
uint32_t timer_get_millis(void);
```

### uart library (key additions for this project)
```c
uint8_t uart_rx_available(void);
char    uart_read_byte(void);
```

## Pin Assignment

| Pin | Function                  | Notes                            |
|-----|---------------------------|----------------------------------|
| PC0 | Application LED (output)  | External LED + 220Ω to GND       |
| PD4 | Button (input, pull-up)   | Onboard button, active-LOW       |
| PD5 | UART TX                   | → USB-serial adapter             |
| PD6 | UART RX                   | ← USB-serial (conflicts PD6 LED) |

> **Note on PD6:** The CH32V003 default UART RX pin (PD6) is shared with the
> onboard LED. Since this project requires full-duplex UART for command input,
> PD6 is configured as UART RX and an external LED on PC0 is used instead.

## Build & Flash

```bash
# From repo root
pio run --target upload
```

## UART Settings

| Parameter | Value |
|-----------|-------|
| Baud rate | 115200 |
| Data bits | 8 |
| Stop bits | 1 |
| Parity    | None |
| Port      | COM* (Windows) or /dev/ttyUSB* (Linux) |

## How to Demo

1. Flash the firmware and open a serial terminal at 115200 baud
2. Observe the startup banner
3. Every 1 second you will see a `[TICK]` line with uptime and LED state
4. Press the onboard button → `[BTN ]` line appears immediately
5. Type a command and press Enter:
   - `help` — lists all commands
   - `status` — shows uptime, LED state, total events dispatched
   - `led on` / `led off` — directly controls LED, shows `[CMD ]` log
   - `queue` — shows current queue depth (should be 0 when idle)
6. Press button rapidly multiple times to see debounce working correctly

## Architecture

```
PRODUCERS          QUEUE              DISPATCHER → HANDLERS
---------          -----              -------------------------
timer_producer()─►                ┐
button_producer()─► [circular  ] ─► dispatch_one() → handler_timer_tick()
uart_producer() ─► [  buffer  ]  │                → handler_button_pressed()
                   [16 events ]  ┘                → handler_uart_cmd()
```