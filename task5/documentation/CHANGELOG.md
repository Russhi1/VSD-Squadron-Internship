# Changelog

Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/)
Versioning: [Semantic Versioning](https://semver.org/spec/v2.0.0.html)

`MAJOR` — breaking API change | `MINOR` — new feature | `PATCH` — bug fix

---

## [1.0.0] — 2026-05-26

### Added

#### GPIO Driver (`gpio.h` / `gpio.c`)

- **`gpio_init(port, pin, mode)`** — Configures output, floating input,
  pull-up input, or pull-down input on GPIOA/C/D. APB2 clock gating is
  internal — the caller never touches `RCC->APB2PCENR` directly, which
  prevents the common mistake of accessing GPIO registers before the clock
  is enabled.

- **`gpio_write(port, pin, value)`** — Drives output HIGH or LOW via
  read-modify-write on `OUTDR`.

- **`gpio_toggle(port, pin)`** — XOR-flips output level without requiring
  the caller to track current state.

- **`gpio_read(port, pin)`** — Reads live level from `INDR`, not `OUTDR`.
  Critical for input pins where `OUTDR` selects pull direction, not output
  state.

- **`gpio_debounce_read(port, pin, samples)`** — Takes N samples with
  ~40 µs gaps. Returns `GPIO_DEBOUNCE_UNSTABLE (0xFF)` if any sample
  disagrees, so the caller retries rather than acting on a bounce
  transition. Keeps debounce self-contained in the driver — no timer
  peripheral required.

- **Constants** — `PORT_A/C/D`, `GPIO_OUTPUT/INPUT/INPUT_PU/INPUT_PD`,
  `GPIO_HIGH/LOW`, `LED_PIN (PD6)`, `BTN_PIN (PD4)`.

---

#### UART Debug Interface (`uart.h` / `uart.c`)

- **`uart_init(baud)`** — Configures USART1 TX-only on PD5.
  `BRR = SystemCoreClock / baud` gives 0.16% error at 115200 baud —
  within the 3% module tolerance. RX intentionally omitted.

- **`uart_send_byte(byte)`** — Blocking TX with TXE polling. Acceptable
  at 115200 baud for debug logging.

- **`uart_print/println(str)`** — String output without and with `\r\n`.
  Kept separate so multi-part log lines can be built without a format
  buffer.

- **`uart_print_num/int(n)`** — Integer-to-ASCII without `printf()`,
  saving ~2 KB of code size on bare-metal GCC targets.

---

#### Timer-Based Scheduling

- **SysTick 1 ms time base** — Compare value 23999 at 24 MHz. ISR
  increments `volatile uint32_t millis`. Uses `WCH-Interrupt-fast`
  attribute to minimise context save overhead. Does not consume TIM1
  or TIM2.

- **`timer_get_millis()`** — Returns current `millis`. Unsigned
  subtraction in hold duration calculation handles the 49.7-day
  wraparound correctly.

---

#### Demo Application (`main.c`)

- **Startup banner** — Structured boot log confirms every peripheral
  without a debugger.

- **Button-to-LED toggle** — Edge detection on debounced input gives
  exactly one toggle per physical press regardless of hold duration.

- **Press and release logging** — Press number, timestamp, LED state,
  and hold duration logged to UART on each event.

- **Zero register access in `main.c`** — All hardware operations go
  through `gpio_*` and `uart_*`. Porting requires only pin constant
  changes in `gpio.h`.

### Changed

None. Initial release.

### Fixed

None. Initial release.

---

## Versioning Policy

| Change type                    | Version bump |
|--------------------------------|--------------|
| Breaking API change            | MAJOR        |
| New driver function or port    | MINOR        |
| Bug fix, no API change         | PATCH        |
| Documentation only             | No bump      |