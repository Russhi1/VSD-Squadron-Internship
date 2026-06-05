# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Version numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html):
`MAJOR.MINOR.PATCH`
- MAJOR — breaking API changes that require caller code to be updated
- MINOR — new features added in a backward-compatible manner
- PATCH — bug fixes that do not change the API

---

## [1.0.0] — 2026-05-26

### Added

#### GPIO Driver (`gpio.h` / `gpio.c`)

- **`gpio_init(port, pin, mode)`** — Configures any pin on GPIOA, GPIOC,
  or GPIOD as push-pull output, floating input, pull-up input, or
  pull-down input. APB2 clock gating is handled internally so the caller
  never needs to touch `RCC->APB2PCENR` directly. This keeps all
  clock management inside the driver and prevents the common mistake of
  accessing GPIO registers before the peripheral clock is enabled.

- **`gpio_write(port, pin, value)`** — Drives an output pin HIGH or LOW
  via a read-modify-write on `OUTDR`. Isolated from `gpio_read()` so that
  output state can be set independently of sampling.

- **`gpio_toggle(port, pin)`** — XOR-flips the current output level
  without requiring the caller to track pin state. Eliminates the
  read-modify-write pattern that application code would otherwise need
  to implement manually.

- **`gpio_read(port, pin)`** — Reads the live electrical level from
  `INDR`. Returns exactly 0 or 1. Reading from `INDR` rather than `OUTDR`
  ensures the function reflects the actual pin voltage, not the last
  written value — critical for input pins where `OUTDR` has a different
  meaning (pull direction selection).

- **`gpio_debounce_read(port, pin, samples)`** — Software debounce filter
  that takes N samples with ~40 µs inter-sample delays. Returns
  `GPIO_DEBOUNCE_UNSTABLE (0xFF)` if any two samples disagree, signalling
  the caller to retry. This removes mechanical bounce from button inputs
  without requiring a hardware timer, keeping the debounce logic
  self-contained within the GPIO driver.

- **Port aliases** — `PORT_A`, `PORT_C`, `PORT_D` added as readable
  wrappers over the vendor HAL register structs.

- **Mode constants** — `GPIO_OUTPUT`, `GPIO_INPUT`, `GPIO_INPUT_PU`,
  `GPIO_INPUT_PD` added to replace magic numbers in `gpio_init()` calls.

- **Logic level constants** — `GPIO_HIGH`, `GPIO_LOW` added so application
  code reads as plain English rather than bare 1s and 0s.

- **Onboard pin aliases** — `LED_PIN (PD6)`, `BTN_PIN (PD4)` added to
  centralise pin assignments. Changing the LED or button pin requires
  editing one line in `gpio.h`, not hunting through `main.c`.

---

#### UART Debug Interface (`uart.h` / `uart.c`)

- **`uart_init(baud)`** — Configures USART1 as a one-way debug logging
  interface on PD5. The baud rate divisor is calculated from
  `SystemCoreClock / baud`, producing 0.16% error at 115200 baud with a
  24 MHz clock — well within the USART module's 3% tolerance. TX-only
  design is intentional: the application only needs to emit log messages,
  not receive commands, so enabling RX would add unused code and pin
  configuration.

- **`uart_send_byte(byte)`** — Blocking single-byte transmit. Polls the
  `TXE` flag in `STATR` before writing to `DATAR`. Blocking is acceptable
  here because log messages are short and the application is not
  time-critical. A non-blocking interrupt-driven buffer would add
  complexity with no observable benefit at 115200 baud.

- **`uart_print(str)`** — Transmits a null-terminated string without a
  trailing newline. Used for partial log lines that are completed by a
  subsequent call.

- **`uart_println(str)`** — Transmits a null-terminated string followed
  by `\r\n`. Used for complete log lines. Separate from `uart_print()`
  so that multi-part log lines can be constructed without allocating a
  format buffer.

- **`uart_print_num(n)`** — Transmits an unsigned 32-bit integer as
  decimal ASCII. Avoids `printf()` and its associated code size overhead
  (~2 KB on GCC for bare-metal targets). The implementation builds digits
  in a local stack buffer and transmits in reverse order.

- **`uart_print_int(n)`** — Transmits a signed 32-bit integer as decimal
  ASCII with a leading `-` for negative values. Implemented as a thin
  wrapper over `uart_print_num()`.

---

#### Timer-Based Scheduling (`systick` in `main.c`)

- **SysTick 1 ms time base** — SysTick configured with a compare value
  of 23999 to fire an interrupt every 1 ms at 24 MHz. The ISR increments
  a global `volatile uint32_t millis` counter. This provides the
  application with a non-blocking time reference without consuming a
  hardware timer peripheral (TIM1 or TIM2).

- **`timer_get_millis()`** — Returns the current `millis` value. Used by
  the application to timestamp press and release events and to compute
  button hold duration. Unsigned subtraction ensures correct results
  across the 49.7-day wraparound boundary.

- **Fast interrupt attribute** — ISR declared with
  `__attribute__((interrupt("WCH-Interrupt-fast")))` to minimise context
  save/restore overhead. At 1 ms intervals, ISR entry and exit overhead
  would otherwise consume a measurable fraction of CPU time.

---

#### Demo Application (`main.c`)

- **Startup banner** — Prints a structured boot log over UART listing
  every peripheral and its configuration. Allows a reviewer to confirm
  correct initialisation without attaching a debugger.

- **Button-to-LED toggle** — Each confirmed button press toggles the LED
  state. Edge detection is performed by comparing the current debounced
  reading against the previous confirmed state (`prev_btn`), ensuring
  exactly one toggle per physical press regardless of how long the button
  is held.

- **Press logging** — On each falling edge, logs the press number,
  millisecond timestamp, and resulting LED state to UART. Press number
  is a `uint32_t` counter that overflows after ~136 years at one press
  per second.

- **Release logging** — On each rising edge, logs the release timestamp
  and hold duration. Hold duration is computed as
  `release_time - press_time` using unsigned subtraction, which is
  correct across `millis` wraparound.

- **Zero direct register access in `main.c`** — All hardware operations
  go through `gpio_*` and `uart_*` API calls. This enforces the
  driver/application separation and means the application is portable
  to any CH32V003 board by changing pin constants in `gpio.h` only.

### Changed

None. Initial release.

### Fixed

None. Initial release.

---

## Versioning Policy

Future releases will follow these rules:

| Change type                              | Version bump |
|------------------------------------------|--------------|
| Breaking change to any public API        | MAJOR        |
| New driver function added                | MINOR        |
| New supported port or pin mode added     | MINOR        |
| Bug fix with no API change               | PATCH        |
| Documentation update only               | No bump      |