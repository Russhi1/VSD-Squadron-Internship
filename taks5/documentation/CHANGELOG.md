# Changelog

All notable changes to this project are documented here.

The format follows [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Version numbers follow [Semantic Versioning](https://semver.org/spec/v2.0.0.html):
`MAJOR.MINOR.PATCH` — MAJOR for breaking API changes, MINOR for new features, PATCH for bug fixes.

---

## [1.0.0] — 2026-05-26

### Added

**GPIO Driver (`gpio.h` / `gpio.c`)**
- `gpio_init()` — configures any pin on GPIOA, GPIOC, or GPIOD as output, floating input, pull-up input, or pull-down input. Handles APB2 clock enabling internally.
- `gpio_write()` — drives an output pin HIGH or LOW via `OUTDR`.
- `gpio_toggle()` — XOR-flips an output pin's current level without a read-modify-write on the full port.
- `gpio_read()` — reads the live logic level from `INDR` for any pin.
- `gpio_debounce_read()` — samples a pin N times with ~40 µs inter-sample delays; returns `GPIO_DEBOUNCE_UNSTABLE (0xFF)` if any two samples disagree, or the stable level if all agree.
- Port aliases: `PORT_A`, `PORT_C`, `PORT_D`.
- Mode constants: `GPIO_OUTPUT`, `GPIO_INPUT`, `GPIO_INPUT_PU`, `GPIO_INPUT_PD`.
- Level constants: `GPIO_HIGH`, `GPIO_LOW`.
- Onboard pin aliases: `LED_PIN (PD6)`, `BTN_PIN (PD4)`.

**UART Driver (`uart.h` / `uart.c`)**
- `uart_init()` — configures USART1 TX on PD5 at a caller-specified baud rate using `SystemCoreClock / baud` divisor.
- `uart_send_byte()` — blocking single-byte transmit with TXE polling.
- `uart_print()` — transmits a null-terminated string.
- `uart_println()` — transmits a null-terminated string with trailing `\r\n`.
- `uart_print_num()` — transmits an unsigned 32-bit integer as decimal ASCII.
- `uart_print_int()` — transmits a signed 32-bit integer as decimal ASCII, with leading `-` for negatives.

**Demo Application (`main.c`)**
- SysTick configured for 1 ms interrupt-driven `millis` counter using `WCH-Interrupt-fast` attribute.
- Boot banner printed to UART on startup listing all initialised peripherals.
- Main polling loop with `gpio_debounce_read()` (5 samples) for button input.
- Falling-edge detection: toggles LED, increments `press_count`, logs press number and timestamp.
- Rising-edge detection: computes and logs button hold duration.
- All hardware interaction in `main.c` exclusively via `gpio_*` and `uart_*` API calls — zero direct register writes.

### Hardware Support
- Target board: VSDSquadron Mini (CH32V003F4U6)
- Core clock: 24 MHz HSI RC (default)
- GPIO ports: GPIOD (primary), GPIOA and GPIOC (supported by driver, not used by demo)
- UART: USART1, TX only, PD5, 115200 baud 8N1

---

## Planned / Future

The following improvements are candidates for a future release and are noted here for engineering transparency:

- **UART RX support:** Add `uart_rx_available()` and `uart_read_byte()` to enable command input from the host terminal (implemented in the Task-4 fork of this driver).
- **Interrupt-driven UART TX:** Replace the blocking TXE poll with a circular TX buffer and TXEIE interrupt for non-blocking log output.
- **Timer driver:** Extract SysTick configuration from `main.c` into a standalone `timer.h` / `timer.c` module.
- **GPIO interrupt support:** Add EXTI configuration to allow button presses to trigger a hardware interrupt rather than relying on polling.
- **`gpio_write` atomicity:** Replace `OUTDR` read-modify-write with `BSHR` (bit set/reset register) writes to make pin control safe from concurrent ISR access.