# Task 4 — Smart Button Controller (Event Queue Demo)

## What It Does

A single push-button produces different LED responses based on how it is pressed, routed through an event queue.

| Gesture | LED Response |
|---|---|
| Short press | 1 blink |
| Double tap | 5 rapid blinks |
| Long press (≥500ms) | Solid ON for 2 s |

## File Structure

```
task4/
├── event_queue.h   — Queue API and event types
├── event_queue.c   — Ring buffer implementation
├── gpio.h          — Pin definitions and GPIO API
├── gpio.c          — Simulated GPIO (maps to CH32V003 registers)
└── main.c          — Demo app: button simulation + dispatcher
```

## How It Works

```
[Button ISR]  →  eq_post()  →  [Ring Buffer]  →  eq_get()  →  [Dispatcher]
                                                                /    |    \
                                                         SHORT  DBL  LONG
                                                         1blink 5x   2s
```

The ISR only posts an event and returns immediately. The main loop consumes and handles it. The queue is the only interface between the two — neither side knows about the other.

## Event Queue

- Ring buffer, capacity 8, fixed memory (no malloc)
- `eq_post()` — ISR-safe, drops event if full (never blocks)
- `eq_get()` — returns oldest event, 0 if empty

## Hardware Pins (CH32V003)

| Resource | Pin | Register |
|---|---|---|
| LED | PD6 | `GPIOD_BSHR @ 0x40011410` |
| Button | PD7 | `GPIOD_INDR @ 0x40011408` |

## Build and Run

```bash
gcc -Wall -Wextra -o smart_button main.c event_queue.c gpio.c
./smart_button
```