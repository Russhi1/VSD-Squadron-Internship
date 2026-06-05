# VSDSquadron Mini — Advanced GPIO & UART Firmware Library

## Summary

This project implements a reusable, register-level firmware library for the **CH32V003F4U6** microcontroller on the VSDSquadron Mini board. It provides a clean, hardware-abstracted API for GPIO control and UART communication, and demonstrates these drivers through a button-controlled LED application with hardware debounce and real-time serial logging.

The library is written in bare-metal C against the CH32V003 register map — no HAL, no RTOS, no external dependencies. Every hardware operation is performed through named API functions, keeping `main.c` free of direct register manipulation. This design mirrors the driver/application separation used in professional embedded firmware projects.

---

## Target Hardware

| Item | Details |
|---|---|
| Board | VSDSquadron Mini |
| MCU | CH32V003F4U6 |
| Architecture | 32-bit RISC-V (QingKe V2A, RV32EC) |
| System Clock | 24 MHz (HSI internal RC, default) |
| Flash | 16 KB |
| SRAM | 2 KB |
| Debug Interface | 1-wire SDI |

---

## Supported Drivers

| Driver | Source Files | Peripheral |
|---|---|---|
| GPIO | `gpio.h`, `gpio.c` | GPIOA, GPIOC, GPIOD |
| UART | `uart.h`, `uart.c` | USART1 (TX only, PD5) |

Both drivers are self-contained. Either can be pulled into a new project independently.

---

## Folder Structure
project/
├── task3/
│   └── submission/
│       ├── app/
│       │   └── main.c
│       └── library/
│           ├── gpio.h
│           ├── gpio.c
│           ├── uart.h
│           ├── uart.c
│           ├── timer.h
│           └── timer.c
└── task5/
    └── documentation/
        ├── README.md
        ├── API_REFERENCE.md
        ├── ARCHITECTURE.md
        ├── APPLICATION_GUIDE.md
        ├── DEMO_GUIDE.md
        └── CHANGELOG.md

**Build and flash:**

```bash
git clone https://github.com/Russhi1/VSD-Squadron-Internship.git
cd VSD-Squadron-Internship/task3/submission
pio run --target upload
pio device monitor --baud 115200
# Press the onboard button — LED toggles, UART logs each event
```

**Open serial monitor at 115200 baud** (8N1, no flow control) on the USB-UART adapter connected to PD5.

**Press the onboard button (PD4).** The LED (PD6) toggles. Press count, timestamp, and hold duration are printed over UART.



---

## Key Design Decisions

- **No direct register writes in `main.c`** — application code is fully decoupled from hardware details.
- **Software debounce in the driver** — `gpio_debounce_read()` absorbs mechanical bounce before the application sees a state change.
- **SysTick for timestamps** — a 1 ms interrupt-driven counter gives the application a real-time reference without using a timer peripheral.
- **UART TX only** — keeps the driver minimal; the board does not need RX for this application.

---

## Document Index

| Document | Purpose |
|---|---|
| [API_REFERENCE.md](API_REFERENCE.md) | Every public function: parameters, return values, usage |
| [ARCHITECTURE.md](ARCHITECTURE.md) | System layers, block diagram, data and control flow |
| [APPLICATION_GUIDE.md](APPLICATION_GUIDE.md) | Application logic, state machine, timing behaviour |
| [DEMO_GUIDE.md](DEMO_GUIDE.md) | Exact steps to flash and verify the demo |
| [CHANGELOG.md](CHANGELOG.md) | Version history |