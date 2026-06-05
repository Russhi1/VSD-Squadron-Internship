# System Architecture

## Overview

Two-layer firmware stack. Drivers sit below the application. The application
communicates with hardware exclusively through driver API calls — no direct
register access in `main.c`.

---

## High-Level Block Diagram

```mermaid
graph TD
    subgraph APP["Application Layer — main.c"]
        A1["Polling Loop"]
        A2["Edge Detection"]
        A3["LED Toggle Logic"]
        A4["UART Logger"]
    end

    subgraph DRV["Driver Layer — Hardware Abstraction"]
        G["gpio.c / gpio.h"]
        U["uart.c / uart.h"]
    end

    subgraph HW["Hardware Layer — CH32V003F4U6 @ 24 MHz"]
        H1["GPIOD\nPD4 — Button\nPD6 — LED"]
        H2["USART1\nPD5 — TX"]
        H3["SysTick\n1 ms ISR"]
    end

    A1 -->|"gpio_debounce_read()"| G
    A2 -->|"falling edge"| A3
    A2 -->|"press / release event"| A4
    A3 -->|"gpio_write()"| G
    A4 -->|"uart_print()"| U
    G -->|"CFGLR / OUTDR / INDR"| H1
    U -->|"BRR / CTLR1 / DATAR"| H2
    A1 -.->|"millis counter"| H3
```

---

## Layer Descriptions

### Hardware Layer

CH32V003F4U6 at 24 MHz from the internal HSI RC oscillator.

- **GPIOD** — PD4 (button, pull-up input), PD5 (UART TX, alt function),
  PD6 (LED, push-pull output).
- **USART1** — Clocked from APB2 at 24 MHz. Baud rate set by `BRR`.
- **SysTick** — Core-private 32-bit counter. Compare value 23999 generates
  a 1 ms interrupt to increment `millis`.

### Driver Layer

**GPIO (`gpio.c`):** Each pin is configured by writing a 4-bit nibble into
`CFGLR`. `gpio_init()` uses a clear-then-set pattern to avoid disturbing
adjacent pins. APB2 clock enabling is handled by a private `enable_clock()`
helper — not exposed in the header because the application has no reason to
manage clocks directly. Debounce is implemented entirely in software within
the driver so any application using a button gets it for free.

**UART (`uart.c`):** USART1 configured TX-only, 8N1, blocking. Polling the
`TXE` flag before each byte write is sufficient at 115200 baud for debug
logging.

### Scheduler / Event System

No RTOS. A single SysTick ISR fires every 1 ms and increments `millis`:

```
SysTick ISR (1 ms)
└── millis++   ← volatile uint32_t
```

Declared with `__attribute__((interrupt("WCH-Interrupt-fast")))` to save
only minimal register context. Clears `SysTick->SR` immediately to prevent
re-entry. `millis` drives press timestamps and hold duration calculation.
GPIO and UART are both polled — no other interrupt sources.

### Application Layer

`main.c` does three things: initialises drivers in the correct order,
runs the polling loop, and detects button edges by comparing the current
debounce result against `prev_btn`.

---

## Data Flow

```mermaid
graph TD
    B1["Button pressed"]
    B2["PD4 drops LOW"]
    B3["gpio_debounce_read — 5 samples"]
    B4{"All samples agree?"}
    B5["Return UNSTABLE\nloop continues"]
    B6["Return GPIO_LOW"]
    B7{"prev_btn was HIGH?"}
    B8["No action"]
    B9["Falling edge confirmed"]
    B10["Toggle LED"]
    B11["Log press to UART"]
    B12["gpio_write PD6"]
    B13["LED changes state"]

    B1 --> B2 --> B3 --> B4
    B4 -->|"No"| B5
    B4 -->|"Yes"| B6 --> B7
    B7 -->|"No"| B8
    B7 -->|"Yes"| B9
    B9 --> B10 --> B12 --> B13
    B9 --> B11
```

---

## Control Flow

```mermaid
graph TD
    S1["main()"]
    S2["systick_init()"]
    S3["uart_init(115200)"]
    S4["gpio_init PD6 OUTPUT"]
    S5["gpio_init PD4 INPUT_PU"]
    S6["gpio_write PD6 LOW"]
    S7["Print banner"]
    S8["while(1)"]
    S9["gpio_debounce_read PD4"]
    S10{"UNSTABLE?"}
    S11["continue"]
    S12{"btn==LOW\nprev==HIGH?"}
    S13["Toggle LED\nLog press"]
    S14{"btn==HIGH\nprev==LOW?"}
    S15["Log release\n+ hold duration"]
    S16["prev_btn = btn"]

    S1 --> S2 --> S3 --> S4 --> S5 --> S6 --> S7 --> S8
    S8 --> S9 --> S10
    S10 -->|"Yes"| S11 --> S8
    S10 -->|"No"| S12
    S12 -->|"Yes"| S13 --> S14
    S12 -->|"No"| S14
    S14 -->|"Yes"| S15 --> S16
    S14 -->|"No"| S16
    S16 --> S8
```

---

## Why This Architecture

**Polling over interrupts:** One button, one LED, one log interface — a
polling loop is simpler and every state transition is visible in one
execution path through `main()`.

**Strict API boundary:** `main.c` never touches registers. Porting to a
different CH32V003 board means changing pin constants in `gpio.h` only.

**Debounce in the driver:** Debounce is a property of how you read a
mechanical input, not what you do with the result. Keeping it in the GPIO
driver means the application doesn't need to implement it.

**Blocking UART:** Log lines are short, the loop tolerates ~3.9 ms per
line at 115200 baud, and a non-blocking TX buffer would add complexity
with no observable benefit.

**SysTick in `main.c`:** SysTick is the application's time reference, not
a reusable peripheral driver. Keeping it in `main.c` reflects that clearly.