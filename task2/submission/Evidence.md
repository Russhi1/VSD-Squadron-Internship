# Task 2 — Evidence Document
**VSDSquadron Mini RISC-V Embedded Firmware Internship**
Author: Rushil Rai

---

## 1. UART Evidence

### Serial Terminal Screenshot

[Expected output to capture:]<img width="1919" height="1079" alt="Screenshot 2026-05-17 151427" src="https://github.com/user-attachments/assets/ad8001ef-1392-44c9-bf95-b4a64ba71894" />


### Serial Terminal Video
[https://drive.google.com/file/d/1l4dPLskBvNlhxPzHHhfJKTqWoAQj0xlw/view?usp=sharing]


---

## 2. GPIO Evidence

### Physical Pin Identification

| Field | Value |
|---|---|
| Physical pin label (silkscreen on board) | **PD6** |
| Port | GPIOD |
| Firmware GPIO number | **6** |
| DataSheet reference | Table 3 — "Built-in LED Pin: 1x onboard user LED (PD6)" |
| Mode configured | GPIO_OUTPUT (push-pull, 50MHz) |
| Toggle period | 500ms |

### Photo — Board with LED Blinking
![Photo of the board]!<img width="9280" height="6944" alt="IMG_20260517_122937 (1)" src="https://github.com/user-attachments/assets/df515fb1-6986-4594-b41b-93a0fc6b107a" />



### Video — LED Blink
> [https://drive.google.com/file/d/1RXXQVeLb_kG5dfMkFSXzIN68Anepsg35/view?usp=sharing]


---

## 3. Verification Explanation

### How correct UART behaviour was verified
1. Flashed firmware via `pio run --target upload`.
2. Opened serial monitor at **115200 baud, 8N1** using `pio device monitor`.
3. On reset, the startup banner appeared immediately — confirming UART initialisation runs before the main loop.
![screenshot of UART]<img width="1707" height="747" alt="Screenshot 2026-05-18 080124" src="https://github.com/user-attachments/assets/2c7158bf-0a73-4100-b965-a0628db47dde" />

4. Counter lines began printing every 500ms and continued indefinitely.
5. The millis timestamp was cross-checked: line N appears at approximately N × 500ms after boot, confirming the SysTick timer is correct at 24MHz.

### How correct GPIO behaviour was verified
1. The LED on PD6 was observed to toggle every 500ms in sync with the UART counter.
2. `Counter: N  LED: ON` lines correspond to odd N (LED physically lit).
3. `Counter: N  LED: OFF` lines correspond to even N (LED physically off).
4. The LED state printed over UART exactly matched the physical LED state, confirming that `gpio_toggle()` and the UART status message are in sync.
5. No direct register writes appear in `main.c` — verified by code review. All pin control goes through `gpio_init()` and `gpio_toggle()` from `gpio.h`.

### Pin mapping verification
- PD6 used in firmware as pin number `6`, port `GPIOD`.
- DataSheet Table 3 confirms: *"Built-in LED Pin: 1x onboard user LED (PD6)"*.
- The silkscreen on the board labels the pin "PD6".
- Numbers match exactly — no invented numbering.
