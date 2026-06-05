# Demo Guide

This guide allows a reviewer to flash the firmware, connect to the serial output, and verify correct behaviour in under 5 minutes. No prior knowledge of the codebase is required.

---

## Prerequisites

| Requirement | Details |
|---|---|
| Hardware | VSDSquadron Mini (CH32V003F4U6) |
| USB-UART adapter | Any CP2102 / CH340 / FT232 adapter |
| Toolchain | PlatformIO installed in VSCode |
| OS | Windows 10+, Ubuntu 20.04+, or macOS 12+ |

---

## Step 1 — Wire the UART Adapter

Connect the USB-UART adapter to the board using three wires:

| VSDSquadron Mini pin | USB-UART adapter pin |
|---|---|
| PD5 (TX) | RX |
| GND | GND |

Do **not** connect the adapter's TX to the board. The firmware is TX-only.

The onboard LED (PD6) and button (PD4) require no external wiring.

---

## Step 2 — Set Up the PlatformIO Project

If you are building from scratch, copy the source files into a PlatformIO project:

```bash
cp task3/submission/library/gpio.h   <project>/include/gpio.h
cp task3/submission/library/gpio.c   <project>/src/gpio.c
cp task3/submission/library/uart.h   <project>/include/uart.h
cp task3/submission/library/uart.c   <project>/src/uart.c
cp task3/submission/app/main.c       <project>/src/main.c
```

Your `platformio.ini` should target the CH32V003 board:

```ini
[env:ch32v003f4u6]
platform = wch-sdk
board = VSDSquadronMini
framework = noneos-sdk
```

---

## Step 3 — Build and Flash

```bash
# Build only
pio run

# Build and flash
pio run --target upload
```

The WCH-Link programmer built into the VSDSquadron Mini handles flashing over the USB connection. If the flash fails, check that the board is powered and the USB cable supports data (not charge-only).

---

## Step 4 — Open Serial Monitor

```bash
pio device monitor --baud 115200
```

Or use any terminal emulator (PuTTY, TeraTerm, screen, minicom) with these settings:

| Setting | Value |
|---|---|
| Baud rate | 115200 |
| Data bits | 8 |
| Parity | None |
| Stop bits | 1 |
| Flow control | None |
| Line ending | CR+LF |

---

## Step 5 — Verify Startup

Immediately after the board powers on or resets, you should see this banner:

```
=========================================
  Advanced GPIO Library — Press Button Demo
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

**What to check:**
- The banner appears within 100 ms of power-on.
- The LED on the board is **OFF** at this point.
- If no output appears, verify the UART wiring and baud rate.
- If garbled text appears, the baud rate is wrong. Set the monitor to exactly 115200.

---

## Step 6 — Press the Button

Press the onboard tactile button once. You should see:

```
[PRESS]   #1  |  t = 3521 ms  |  LED = ON
```

**What to check:**
- The LED on the board turns **ON** at the same moment this line appears.
- The press number starts at 1 and increments by 1 with each press.
- The timestamp (`t = XXXX ms`) shows time since boot in milliseconds.
- The LED field shows `ON` after odd-numbered presses, `OFF` after even-numbered presses.
- Exactly **one** `[PRESS]` line appears per physical button press, even if you press the button firmly and hold it. This confirms the debounce filter is working.

---

## Step 7 — Release the Button

Release the button after holding it for 1–2 seconds. You should see:

```
[RELEASE]     |  t = 5104 ms  |  held = 1583 ms
```

**What to check:**
- The release timestamp is later than the press timestamp.
- The hold duration (`held = XXXX ms`) is the difference between the two timestamps and should match the actual time you held the button.
- No `[PRESS]` line appears during the hold — the application correctly ignores the stable low level between the falling and rising edges.

---
### Expected UART Output
 
```
[PRESS]   #1  |  t = 3521 ms  |  LED = ON
[RELEASE]     |  t = 4638 ms  |  held = 1117 ms
 
[PRESS]   #2  |  t = 5902 ms  |  LED = OFF
[RELEASE]     |  t = 7083 ms  |  held = 1181 ms
 
[PRESS]   #3  |  t = 8347 ms  |  LED = ON
[RELEASE]     |  t = 9512 ms  |  held = 1165 ms
```
 
> Timestamps and hold durations will differ — exact values depend on
> when you press the button.
 
### What to Observe on Hardware
 
| Action                  | Expected hardware observation                          |
|-------------------------|--------------------------------------------------------|
| Press #1                |  LED (PD6) turns **ON** immediately               |
| Release #1              | LED stays **ON** — release does not change LED state   |
| Press #2                |  LED (PD6) turns **OFF** immediately              |
| Release #2              | LED stays **OFF**                                      |
| Press #3                |  LED (PD6) turns **ON** immediately               |
| Hold button down        | LED stays in current state — no flicker during hold    |
| Release button slowly   | No LED change on release                               |
 
### What to Check in UART Output
 
- Press number increments by exactly 1 per physical press
- Each `[PRESS]` timestamp is greater than the previous `[RELEASE]` timestamp
- Hold duration = release timestamp minus press timestamp
- LED field alternates: `ON`, `OFF`, `ON`, `OFF`...
- Exactly **one** `[PRESS]` line per physical button press — confirms debounce is working
---
 
## Step 8 — Verify Debounce
 
Press and release the button 5 times as quickly as possible (rapid taps).
 
**Expected:** Exactly one `[PRESS]` and one `[RELEASE]` line per physical tap.
 
**Not expected:** Multiple `[PRESS]` lines for a single tap.
 
Example of correct debounce output for 3 rapid taps:
 
```
[PRESS]   #4  |  t = 12001 ms  |  LED = OFF
[RELEASE]     |  t = 12088 ms  |  held = 87 ms
[PRESS]   #5  |  t = 12305 ms  |  LED = ON
[RELEASE]     |  t = 12390 ms  |  held = 85 ms
[PRESS]   #6  |  t = 12601 ms  |  held = ON
[RELEASE]     |  t = 12689 ms  |  held = 88 ms
```
 
If you see duplicate `[PRESS]` lines for a single tap, increase the
`samples` argument in `gpio_debounce_read()` from 5 to 10 in `main.c`.
 
---
 
## Complete Expected Session
 
Below is a complete reference output for a full demo session — banner,
three presses, three releases:
 
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
[PRESS]   #1  |  t = 3521 ms  |  LED = ON
[RELEASE]     |  t = 4638 ms  |  held = 1117 ms
[PRESS]   #2  |  t = 5902 ms  |  LED = OFF
[RELEASE]     |  t = 7083 ms  |  held = 1181 ms
[PRESS]   #3  |  t = 8347 ms  |  LED = ON
[RELEASE]     |  t = 9512 ms  |  held = 1165 ms
```
 
---
 
## Troubleshooting
 
| Symptom                          | Likely Cause                        | Fix                                                    |
|----------------------------------|-------------------------------------|--------------------------------------------------------|
| No output in terminal            | Wrong COM port or baud rate         | Check device manager / `ls /dev/tty*`, set 115200      |
| Garbled characters               | Baud rate mismatch                  | Set terminal to exactly 115200                         |
| LED stays OFF after press        | Wrong pin in gpio.h                 | Verify `LED_PIN = 6` in `gpio.h`                       |
| Multiple `[PRESS]` per tap       | Debounce not filtering              | Increase `samples` in `gpio_debounce_read()` to 10     |
| Flash fails — device not found   | Charge-only USB cable               | Replace with a data-capable USB cable                  |
| Flash fails — programmer error   | WCH-Link not detected               | Check USB connection, try different USB port           |
| Banner repeats multiple times    | Board resetting in a loop           | Check USB cable quality — weak cable causes brown-out  |
| LED turns ON at startup          | `gpio_write` call missing           | Verify `gpio_write(PORT_D, LED_PIN, GPIO_LOW)` in main |
| No `[RELEASE]` lines             | Rising edge detection not working   | Check `prev_btn` initialisation in main.c              |
 