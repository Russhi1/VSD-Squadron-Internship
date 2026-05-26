# Task 3 — Advanced GPIO Library + Application Demo

**Board:** VSDSquadron Mini (CH32V003F4U6, 32-bit RISC-V)  
**Toolchain:** PlatformIO + NoneOS-SDK  
**Library:** Advanced GPIO Driver + UART Driver  
**Application:** Button-controlled LED with debounce and UART logging

---

## Library: Advanced GPIO

| File | Purpose |
|---|---|
| `library/gpio.h` | API declarations, constants, pin aliases |
| `library/gpio.c` | Register-level implementation |

| Function | Description |
|---|---|
| `gpio_init(port, pin, mode)` | Enable port clock and configure pin direction |
| `gpio_write(port, pin, value)` | Drive pin HIGH or LOW |
| `gpio_toggle(port, pin)` | Flip current pin level |
| `gpio_read(port, pin)` | Return live logic level of input pin |
| `gpio_debounce_read(port, pin, samples)` | Read pin with bounce filtering — returns `GPIO_DEBOUNCE_UNSTABLE` (0xFF) if still bouncing |

**Pin modes:** `GPIO_OUTPUT`, `GPIO_INPUT`, `GPIO_INPUT_PU`, `GPIO_INPUT_PD`

---

## Supporting Library: UART Driver

| File | Purpose |
|---|---|
| `library/uart.h` | API declarations |
| `library/uart.c` | USART1 implementation, TX only on PD5 |

| Function | Description |
|---|---|
| `uart_init(baud)` | Configure USART1 at given baud rate |
| `uart_print(str)` | Send a string |
| `uart_println(str)` | Send a string with CR+LF |
| `uart_print_num(n)` | Send unsigned integer as decimal |
| `uart_print_int(n)` | Send signed integer as decimal |

---

## Demo Application

**File:** `app/main.c`

The application reads the onboard button (PD4) using `gpio_debounce_read()` with 5 samples to filter mechanical bounce. On every button press, the LED (PD6) is toggled via `gpio_write()` and the event is logged over UART with a press counter, timestamp, and current LED state. On release, the hold duration is logged.

`main.c` contains zero direct register writes — all hardware interaction goes through `gpio_*` and `uart_*` APIs only.

### Hardware connections

| Signal | Pin | Notes |
|---|---|---|
| LED | PD6 | Onboard, no wiring needed |
| Button | PD4 | Onboard, no wiring needed |
| UART TX | PD5 | Connect to USB-UART adapter RX |
| GND | GND | Connect to USB-UART adapter GND |

**Serial monitor:** 115200 baud, 8N1

---

## Build and Flash

Copy library files into PlatformIO build directories:

```
library/gpio.h  →  include/gpio.h
library/gpio.c  →  src/gpio.c
library/uart.h  →  include/uart.h
library/uart.c  →  src/uart.c
app/main.c      →  src/main.c
```

Build and flash using the ✓ button in VSCode or:
```bash
pio run --target upload
```

Open serial monitor at **115200 baud** to see output.

---

## Repository Structure

```
task3/
└── submission/
    ├── library/
    │   ├── gpio.h
    │   ├── gpio.c
    │   ├── uart.h
    │   └── uart.c
    ├── app/
    │   └── main.c
    ├── README.md
    └── evidence.md
```