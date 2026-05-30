# Evidence — Project-9: Event Queue Framework + Demo App
# VSD Squadron Mini (CH32V003F4U6)

---

## UART Log

Captured via PlatformIO Serial Monitor at 115200 baud.
Board: VSDSquadron Mini 

<img width="1482" height="572" alt="Screenshot 2026-05-26 111646" src="https://github.com/user-attachments/assets/5cf677c2-6bc7-45ac-9528-4c20b1d45474" />
<img width="672" height="748" alt="Screenshot 2026-05-29 122043" src="https://github.com/user-attachments/assets/fd9473d0-9b5d-468c-a92c-ab62d3231532" />


## Hardware Verification

Video Demo link [https://drive.google.com/file/d/1jl1jrn7Y9BOLq5asMd16z96OFVaTnE_8/view?usp=sharing]



## Verification Notes

### What Was Tested
- Timer producer fires every 1000ms, TICK event dispatched with correct timestamp
- Button debounce confirmed — rapid presses produce single events only
- All five UART commands tested: `help`, `status`, `led on`, `led off`, `queue`
- Priority ordering confirmed in logs — HIGH before NORM before LOW
- Queue depth shows 0 when idle, increments briefly under burst input
- Invalid command input handled gracefully with error message

### What Worked
- Circular buffer push and pop with correct wrap-around at index 16
- Priority-aware dispatch — UART commands always handled before timer ticks
- Producer and consumer fully separated — main.c contains zero register access
- Debounced button — mechanical bounce eliminated, one press = one event
- `dropped=0` throughout all testing — no queue overflow under normal load
- Startup banner appears immediately on power-on confirming UART is live

### Limitations
- **PD6 pin conflict:** UART RX shares the onboard LED pin. External LED
  on PC0 used instead. This is a CH32V003 hardware constraint, not a bug.
- **Command length:** Input capped at 23 characters. Longer input silently
  truncated. All supported commands are well within this limit.
- **No persistent state:** Counters and LED state reset on every power cycle.
- **Single-threaded:** A blocking handler would delay all other events.
  Not observed in testing as all handlers complete in under 1ms.
