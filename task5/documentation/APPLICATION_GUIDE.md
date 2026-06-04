# Application Guide

This document explains what the demo application does, how its logic is structured, and why each design decision was made. It does not repeat the driver API — see `API_REFERENCE.md` for that.

---

## What the Application Does

The application turns the CH32V003's onboard button (PD4) into a toggle switch for the onboard LED (PD6). Each confirmed button press flips the LED state. Every press and release is logged to a host terminal over UART with a timestamp (milliseconds since boot) and, on release, the duration the button was held.

---

## Initialisation Sequence

The application initialises peripherals in this specific order:

```
1. systick_init()       — 1 ms time base must be active before any timing logic
2. uart_init(115200)    — UART ready before any log message is printed
3. gpio_init(PD6, OUTPUT)   — LED pin configured
4. gpio_init(PD4, INPUT_PU) — Button pin configured with pull-up
5. gpio_write(PD6, LOW)     — LED explicitly set OFF at startup
6. Banner printed to terminal
```

Order matters. SysTick is initialised first because the `millis` variable it drives is read in log messages immediately after `uart_init()`. If UART were initialised before SysTick, the banner timestamps would read 0, but more importantly, if SysTick were initialised after GPIO, a brief window would exist where the LED pin is configured but interrupts are not yet enabled — harmless here, but bad practice.

---

## Application State

The application tracks two pieces of state between loop iterations:

| Variable | Type | Purpose |
|---|---|---|
| `prev_btn` | `uint8_t` | Last confirmed (debounced) button level; used for edge detection |
| `led_state` | `uint8_t` | Current LED level; toggled on each press |
| `press_count` | `uint32_t` | Cumulative count of confirmed presses since boot |
| `press_time` | `uint32_t` | `millis` value captured at the moment of each press; used to compute hold duration on release |

There is no formal state machine with named states. The logic is simple enough to express as two `if` conditions on edge direction.

---

## Main Loop Logic

The main loop runs continuously and unconditionally. Each iteration:

```
1. Call gpio_debounce_read(PORT_D, BTN_PIN, 5)
   → result is UNSTABLE, GPIO_HIGH, or GPIO_LOW

2. If UNSTABLE → continue (skip remaining logic this iteration)

3. If btn == GPIO_LOW AND prev_btn == GPIO_HIGH
   → falling edge = button just pressed
   → increment press_count
   → record press_time = millis
   → toggle led_state
   → call gpio_write() with new led_state
   → log: press number, timestamp, LED state

4. If btn == GPIO_HIGH AND prev_btn == GPIO_LOW
   → rising edge = button just released
   → compute hold = millis - press_time
   → log: timestamp, hold duration

5. prev_btn = btn  (update for next iteration)
```

The loop has no `delay()`. It runs as fast as the debounce read allows (~160 µs per call when stable). This means the button is sampled approximately 6000 times per second when not bouncing.

---

## Timing Behaviour

### SysTick

The SysTick interrupt fires every 1 ms and increments `millis`. The ISR is declared with `__attribute__((interrupt("WCH-Interrupt-fast")))`, which is WCH's vendor-specific fast-interrupt attribute. It causes the core to save only the minimal register context, reducing ISR entry/exit overhead to a few cycles. The ISR clears `SysTick->SR` (the compare match flag) immediately to prevent re-entry.

### Debounce timing

Each call to `gpio_debounce_read()` with `samples = 5` takes approximately 160–170 µs of blocking execution. This time is dominated by four busy-wait gaps of 1000 loop iterations each at 24 MHz. During this time, no other work is done and `millis` continues incrementing normally (SysTick is interrupt-driven and is not blocked by the busy-wait).

### UART transmission time

At 115200 baud, each character takes approximately 87 µs. A typical log line such as `[PRESS]   #3  |  t = 12450 ms  |  LED = ON` is 45 characters, taking approximately 3.9 ms to transmit. During this time, the main loop is blocked inside `uart_print*` calls. This means a press event that arrives while a log line is being transmitted will be detected on the next iteration after the UART call returns, with a timestamping error of at most ~4 ms. This is acceptable for a human-interaction demo.

---

## Edge Cases Handled

### Mechanical bounce

`gpio_debounce_read()` returns `GPIO_DEBOUNCE_UNSTABLE` while the pin is in a bouncing state. The main loop checks for this and calls `continue`, skipping both edge detection and `prev_btn` update. This means bounce transitions are invisible to the application: `prev_btn` retains its last confirmed value until the pin settles.

Without this, a single button press could easily generate 5–20 rapid HIGH/LOW transitions, each triggering a toggle, leaving the LED in an unpredictable state.

### Rapid repeated presses

Each call to `gpio_debounce_read()` is independent. If the button is pressed and released in under 1 ms (faster than human capability), the falling and rising edges will both be detected on successive loop iterations, provided the debounce samples agree on each call. In practice, human presses last at least 50 ms, so this is not a real concern.

### First press after boot

`prev_btn` is initialised to `GPIO_HIGH` (released state). This is correct because the button is wired with a pull-up: when not pressed, PD4 reads HIGH. If `prev_btn` were initialised to `GPIO_LOW`, the first debounce read returning HIGH would trigger a false "release" event before any button was ever pressed.

### Counter overflow

`press_count` is `uint32_t`, giving a range of 0 to 4,294,967,295 presses. At one press per second continuously, this would overflow in approximately 136 years. No overflow handling is needed.

### `millis` overflow

`millis` is `volatile uint32_t` and will wrap at 4,294,967,295 ms (approximately 49.7 days of uptime). The hold duration calculation `millis - press_time` uses unsigned subtraction, which gives the correct result even across a wrap boundary (e.g., if `millis = 5` and `press_time = 4294967290`, the result is `5 - 4294967290 = 11` in unsigned 32-bit arithmetic, which is correct). Timestamps in log messages will reset to small values after wraparound, which is expected and not harmful.