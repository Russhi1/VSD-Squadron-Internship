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

## Step 8 — Verify Debounce

To confirm debounce is working, press and release the button very quickly 5 times in rapid succession. Each press should produce exactly one `[PRESS]` line and each release one `[RELEASE]` line. You should **not** see multiple `[PRESS]` lines for a single physical press.

Expected output for 3 quick presses:

```
[PRESS]   #1  |  t = 4001 ms  |  LED = ON
[RELEASE]     |  t = 4088 ms  |  held = 87 ms
[PRESS]   #2  |  t = 4205 ms  |  LED = OFF
[RELEASE]     |  t = 4290 ms  |  held = 85 ms
[PRESS]   #3  |  t = 4401 ms  |  LED = ON
[RELEASE]     |  t = 4489 ms  |  held = 88 ms
```

If you see duplicated `[PRESS]` lines, the debounce is not effective for your specific button. Increase the `samples` parameter in `main.c` from 5 to 10 or higher.

---

## Troubleshooting

| Symptom | Likely cause | Fix |
|---|---|---|
| No output in terminal | Wrong COM port or baud rate | Check device manager / `ls /dev/tty*`, set 115200 |
| Garbled characters | Baud rate mismatch | Set terminal to exactly 115200 |
| LED stays OFF after press | GPIO driver not writing PD6 | Verify `LED_PIN = 6` in `gpio.h` |
| Multiple `[PRESS]` per tap | Button bounce not filtered | Increase `samples` argument in `gpio_debounce_read()` call |
| Flash fails | USB cable or programmer issue | Try a different USB cable; check WCH-Link LED |
| Banner repeats multiple times | Board is resetting in a loop | Check power supply; a weak USB cable can cause brown-out resets |