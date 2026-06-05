# Demo Guide

This guide allows a reviewer to flash the firmware, connect to the serial output, and verify correct behaviour in under 5 minutes. No prior knowledge of the codebase is required.



---

## Prerequisites

| Requirement      | Details                                                      |
|------------------|--------------------------------------------------------------|
| Hardware         | VSDSquadron Mini (CH32V003F4U6)                              |
| USB cable        | Data-capable USB-C — charge-only cables will not work        |
| USB-UART adapter | CP2102 / CH340 / FT232                                       |
| Toolchain        | VSCode with PlatformIO extension                             |
| OS               | Windows 10+, Ubuntu 20.04+, macOS 12+                        |

---

## Step 1 — Wire the UART Adapter

| VSDSquadron Mini | USB-UART adapter |
|------------------|------------------|
| PD5 (TX)         | RX               |
| GND              | GND              |

Do **not** connect adapter TX. Firmware is TX-only.
LED (PD6) and button (PD4) need no external wiring.

---

## Step 2 — Clone and Configure

```bash
git clone https://github.com/Russhi1/VSD-Squadron-Internship.git
cd VSD-Squadron-Internship/task3/submission
```

Verify `platformio.ini`:

```ini
[env:ch32v003f4u6]
platform = wch-sdk
board = VSDSquadronMini
framework = noneos-sdk
```

---

## Step 3 — Build and Flash

```bash
pio run --target upload
```

If flash fails, check the troubleshooting table below.

---

## Step 4 — Open Serial Monitor

```bash
pio device monitor --baud 115200
```

| Setting      | Value  |
|--------------|--------|
| Baud rate    | 115200 |
| Data bits    | 8      |
| Parity       | None   |
| Stop bits    | 1      |
| Flow control | None   |
| Line ending  | CR+LF  |

---

## Step 5 — Verify Startup

Reset the board. Expected within 100 ms:

```
=========================================
  Advanced GPIO Library - Press Button Demo
  VSDSquadron Mini | CH32V003F4U6
=========================================
[INIT] SysTick  : 1 ms tick active
[INIT] UART     : PD5, 115200 baud, 8N1
[INIT] LED      : PD6, output, starts OFF
[INIT] Button   : PD4, pull-up, active LOW
[INIT] Debounce : 5 samples x ~40 us each
-----------------------------------------
[INFO] Press the button to toggle the LED
-----------------------------------------
```

**On hardware:** LED (PD6) is OFF. Banner appears exactly once.

---

## Step 6 — Press and Release the Button

Press the button three times, holding ~1 second each.

**Expected UART output:**

```
[PRESS]   #1  |  t = 3521 ms  |  LED = ON
[RELEASE]     |  t = 4638 ms  |  held = 1117 ms
[PRESS]   #2  |  t = 5902 ms  |  LED = OFF
[RELEASE]     |  t = 7083 ms  |  held = 1181 ms
[PRESS]   #3  |  t = 8347 ms  |  LED = ON
[RELEASE]     |  t = 9512 ms  |  held = 1165 ms
```

Timestamps will differ — values depend on when you press.

**On hardware:**

| Action    | Expected                                       |
|-----------|------------------------------------------------|
| Press #1  | LED turns ON immediately                       |
| Release   | LED stays ON — release does not change state   |
| Press #2  | LED turns OFF immediately                      |
| Press #3  | LED turns ON immediately                       |
| Hold down | LED holds state — no flicker                   |

---

## Step 7 — Verify Debounce

Tap the button 5 times quickly. Each tap must produce exactly one
`[PRESS]` and one `[RELEASE]` line. Multiple `[PRESS]` lines for a
single tap means debounce is not filtering — increase `samples` in
`gpio_debounce_read()` from 5 to 10 in `main.c`.

---

## Troubleshooting

| Symptom                       | Likely Cause             | Fix                                          |
|-------------------------------|--------------------------|----------------------------------------------|
| No output                     | Wrong port or baud rate  | Check device manager, set exactly 115200     |
| Garbled characters            | Baud rate mismatch       | Set terminal to exactly 115200               |
| LED stays OFF after press     | Wrong pin in gpio.h      | Verify `LED_PIN = 6`                         |
| Multiple `[PRESS]` per tap    | Debounce insufficient    | Increase `samples` to 10                     |
| Flash fails — not found       | Charge-only USB cable    | Replace with data-capable cable              |
| Flash fails — programmer error| WCH-Link not detected    | Try different USB port                       |
| Banner repeats                | Board resetting in loop  | Weak USB cable causing brown-out — replace   |
| No `[RELEASE]` lines          | Rising edge not detected | Check `prev_btn` initialisation in `main.c`  |