# System Architecture

## Overview

The project is structured as a two-layer firmware stack. The bottom layer contains hardware drivers. The top layer contains the application. The two layers communicate only through the driver API; the application never accesses hardware registers directly.


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




## Layer Descriptions

### Hardware Layer

The CH32V003F4U6 is a 32-bit RISC-V microcontroller running at 24 MHz from its internal HSI RC oscillator. The peripherals used in this project are:

- **GPIOD:** Three pins are used. PD4 reads the button (input with pull-up), PD5 drives the UART TX line (alternate function), and PD6 drives the onboard LED (push-pull output).
- **USART1:** Clocked from APB2 (same as HCLK = 24 MHz in this configuration). Baud rate is set by dividing this clock by a value written to `BRR`.
- **SysTick:** A 32-bit up-counter built into the RISC-V core. Configured with a compare value of 23999 (24000 - 1) to generate a match interrupt every 1 ms.

### Driver Layer

**GPIO driver (`gpio.c`):**

The CH32V003 configures each GPIO pin using 4 bits inside the `CFGLR` register. The `gpio_init()` function calculates the correct nibble for each requested mode and writes it via a clear-then-set pattern to avoid disturbing adjacent pins. The enable function for APB2 clocks is handled by a private `enable_clock()` helper inside `gpio.c`; it is not exposed in the header because the application has no reason to manage clocks directly.

The debounce filter (`gpio_debounce_read`) is implemented entirely in software using repeated calls to the base `gpio_read()` with busy-wait delays between samples. No hardware timer is used for debounce. This is a deliberate design choice: it keeps the debounce logic self-contained within the GPIO driver without requiring a timer peripheral, and the blocking time (~160 µs) is acceptable in a polling loop.

**UART driver (`uart.c`):**

USART1 is configured in the simplest possible way for one-way debug output: transmitter only, 8 data bits, no parity, 1 stop bit (8N1), blocking transmission. All `uart_print*` functions call down to `uart_send_byte()`, which polls the `TXE` flag in `STATR` before writing each byte to `DATAR`. There is no interrupt-driven TX buffer or DMA — the UART is only used for logging at human-readable rates, so blocking is acceptable.

**SysTick (in `main.c`, not a separate driver):**

SysTick is configured directly in `main.c` rather than as a library module. This is intentional: SysTick is a core-private peripheral belonging to the application, not a reusable driver in the same way as GPIO or UART. The `millis` counter it increments is a global `volatile uint32_t`.

### Application Layer

`main.c` contains all application logic. It has three responsibilities:

1. **Initialise all drivers** in the correct order (SysTick first, then UART so the banner can be printed, then GPIO).
2. **Run the polling loop**, which calls `gpio_debounce_read()` every iteration to sample the button.
3. **Detect edges** by comparing the current debounce result against the previous confirmed state, and acting on falling edges (press) and rising edges (release).

---

## Data Flow
```mermaid
graph TD
    B1["Button pressed physically"]
    B2["PD4 voltage drops LOW"]
    B3["gpio_debounce_read\nsamples pin 5 times"]
    B4{"All 5 samples\nagree?"}
    B5["Return UNSTABLE\nloop continues"]
    B6["Return GPIO_LOW"]
    B7{"prev_btn\nwas HIGH?"}
    B8["No action taken"]
    B9["Falling edge confirmed"]
    B10["Toggle LED state"]
    B11["Log press to UART"]
    B12["gpio_write updates PD6"]
    B13["LED turns ON or OFF"]

    B1 --> B2 --> B3 --> B4
    B4 -->|"No"| B5
    B4 -->|"Yes"| B6
    B6 --> B7
    B7 -->|"No"| B8
    B7 -->|"Yes"| B9
    B9 --> B10
    B9 --> B11
    B10 --> B12
    B12 --> B13
```

# Control Flow

```mermaid
graph TD
    S1["main()"]
    S2["systick_init()"]
    S3["uart_init(115200)"]
    S4["gpio_init PD6 OUTPUT"]
    S5["gpio_init PD4 INPUT_PU"]
    S6["gpio_write PD6 LOW\nLED starts OFF"]
    S7["Print startup banner"]
    S8["while(1) — polling loop"]
    S9["gpio_debounce_read\nPD4, 5 samples"]
    S10{"Result ==\nUNSTABLE?"}
    S11["continue\nskip this iteration"]
    S12{"btn==LOW\nprev==HIGH?"}
    S13["Falling edge\nPress detected"]
    S14["Toggle LED\nLog press + timestamp"]
    S15{"btn==HIGH\nprev==LOW?"}
    S16["Rising edge\nRelease detected"]
    S17["Log release\n+ hold duration"]
    S18["prev_btn = btn"]

    S1 --> S2 --> S3 --> S4 --> S5 --> S6 --> S7 --> S8
    S8 --> S9 --> S10
    S10 -->|"Yes"| S11 --> S8
    S10 -->|"No"| S12
    S12 -->|"Yes"| S13 --> S14 --> S15
    S12 -->|"No"| S15
    S15 -->|"Yes"| S16 --> S17 --> S18
    S15 -->|"No"| S18
    S18 --> S8
```


## Why This Architecture Was Chosen

**No RTOS, no interrupts for GPIO/UART:** At this project's scale (one button, one LED, one serial log), a cooperative polling loop is simpler, more predictable, and easier to debug than an interrupt-driven design. Every state transition is visible in a single execution path through `main()`.

**Strict API boundary:** By banning direct register access from `main.c`, the driver can be ported to a different CH32V003 board simply by changing pin number constants in `gpio.h`. The application code requires no modification.

**Blocking UART:** Blocking TX is appropriate here because the application is not time-critical. Log lines are short, UART runs at 115200 baud, and the polling loop easily tolerates the ~0.9 ms it takes to transmit a typical 12-character log line. A non-blocking UART with an interrupt-driven TX buffer would add complexity without any observable benefit.

**Software debounce in the driver, not the application:** Debounce is a property of how you read a mechanical input, not a property of what you do with the reading. Placing `gpio_debounce_read()` inside the GPIO driver means any future application that uses a button gets debounce for free.
