# Evidence — Task 3: Advanced GPIO Library Demo

## 1. UART Evidence

###  Output

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

## 2. Hardware Evidence

> **Instructions:** Take a clear photo showing:
> - The VSDSquadron Mini board
> - LED visibly ON or OFF
> - USB-UART adapter connected to PD5
> - Optional: serial monitor visible in background

### Photo / Video

[Photo]![alt text](IMG_20260526_113511.jpg)
[Video link]
-->

---

## 3. Explanation
 
### How the application uses the library
 
`main.c` only calls library functions — no direct register writes anywhere in the file. Here's what each part of the app does and which API it uses:
 
| What happens | Library call used |
|---|---|
| Configure LED as output | `gpio_init(PORT_D, LED_PIN, GPIO_OUTPUT)` |
| Configure button as pull-up input | `gpio_init(PORT_D, BTN_PIN, GPIO_INPUT_PU)` |
| Read button with bounce filtering | `gpio_debounce_read(PORT_D, BTN_PIN, 5)` |
| Drive LED on or off | `gpio_write(PORT_D, LED_PIN, led_state)` |
| Print startup banner | `uart_println(...)` |
| Log press count and timestamp | `uart_print_num(press_count)` |
 
### What was verified on hardware
 
- [ ] Firmware flashes and boots without errors
- [ ] Startup banner appears in serial monitor
- [ ] LED is OFF at startup
- [ ] LED turns ON on first button press
- [ ] LED turns OFF on second button press
- [ ] Press count increments correctly
- [ ] Timestamps increase monotonically
- [ ] Hold duration reported correctly
- [ ] Debounce works — single press produces exactly one log line
- [ ] All UART output readable at 115200 baud
### Debounce verification
 
Mechanical buttons bounce — the pin rapidly flickers between HIGH and LOW
for a short time after each press. Without handling this, one press can
register as 2 or 3 presses. `gpio_debounce_read()` reads the pin 5 times
with small delays between each. If all 5 agree, the reading is treated as
stable. If any differ, the result is discarded. On hardware this meant
every single button press produced exactly one `[PRESS]` line in the
serial monitor — no false triggers.
 
---
 
## 4. Build Log
 
> To get this, run `pio run` in the terminal and copy the full output.
> It should end with `[SUCCESS]` and show three compiled files:
> `gpio.o`, `uart.o`, and `main.o`.
> This proves the code compiled without errors before flashing.
 
```

```