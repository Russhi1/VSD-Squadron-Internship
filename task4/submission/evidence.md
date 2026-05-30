# Evidence — Project-9: Event Queue Framework + Demo App
# VSD Squadron Mini (CH32V003F4U6)

---

## UART Log

Captured via PlatformIO Serial Monitor at 115200 baud.
Board: VSDSquadron Mini 

================================
  Event Queue Framework 
  Project-9  VSDSquadron Mini
================================
  Producers: TIMER | BTN | UART
================================

!(<Screenshot 2026-05-29 122043.png>)
!(<Screenshot 2026-05-29 122051.png>)

## Hardware Verification

Video Demo link [https://drive.google.com/file/d/1jl1jrn7Y9BOLq5asMd16z96OFVaTnE_8/view?usp=sharing]



## Brief Verification Notes

### What Was Tested

#### 1. Timer Producer and TICK Event
- Observed `[LOW ][TICK]` lines appearing in the terminal
- Checked that the timestamp increments by approximately 1000ms
  between consecutive TICK events
- Verified that uptime counter increments by 1 each tick
- Verified that the LED toggles state on every tick, producing
  a visible 1-second blink on the external LED at PC0

#### 2. Button Producer and BUTTON_PRESSED Event
- Pressed the onboard button (PD4) multiple times at different speeds
- Each confirmed press produced exactly one `[NORM][BTN ]` log line
- Rapid successive presses (faster than 50ms) were correctly absorbed
  by the debounce logic and did not produce duplicate events
- Verified that button press correctly toggles LED state independent
  of the timer heartbeat
- Confirmed that the event carries a correct timestamp reflecting
  the actual moment of press

#### 3. UART Producer and UART_CMD Event
- Sent each command from the terminal and confirmed correct response:
  - `help`    → printed the full command list
  - `status`  → showed correct uptime, LED state, event count
  - `led on`  → LED turned on immediately, log confirmed
  - `led off` → LED turned off immediately, log confirmed
  - `queue`   → showed depth=0/16 when no events were pending
- Sent an invalid command (`"abc"`) → system responded with
  `[ERR] unknown command` and suggested typing 'help'
- Verified that partial input (typing without pressing Enter) does
  not trigger spurious events — command only fires on newline

#### 4. Priority-Aware Dispatch
- Verified that `[HIGH]` CMD events appear before `[NORM]` BTN events
  and `[LOW ]` TICK events in the log when multiple events are
  queued in the same loop cycle
- Confirmed priority labels are correct in every log line:
  - TICK      → LOW
  - BTN press → NORM
  - CMD       → HIGH

#### 5. Queue Depth and Overflow Protection
- Ran `queue` command while idle: reported `depth=0/16`
- Pressed button and sent command rapidly in sequence:
  queue depth briefly showed 1-2 before draining to 0
- `status` showed `dropped=0` throughout all testing,
  confirming no queue overflow occurred under normal use

#### 6. Event Latency
- Under normal single-event load, latency between enqueue and
  dispatch was 0–1ms (confirmed by enqueue_time tracking in code)
- No blocking delays exist anywhere in the main loop, keeping
  the system responsive at all times

#### 7. API Boundary Compliance
- Reviewed main.c: confirmed zero direct register accesses
  (no GPIOD->, no USART1->, no RCC-> anywhere in application code)
- All hardware interaction goes through gpio_*, uart_*, timer_* APIs
- eventq_push() and eventq_pop() are the only communication path
  between producers and handlers

---

### What Worked

- Event queue circular buffer: push and pop operate correctly
  with wrap-around at index 16
- Priority-aware pop: highest-priority event is always selected
  first regardless of insertion order
- Debounced button: mechanical bounce eliminated, single physical
  press always produces exactly one BUTTON_PRESSED event
- UART command parsing: newline-terminated strings correctly
  assembled character by character from non-blocking RX polling
- Timer 1ms tick: SysTick ISR fires reliably, millis counter
  increments without drift over 10+ second observation window
- LED control: responds correctly to both timer heartbeat and
  direct UART commands with no conflict
- Startup banner: appears immediately on power-on or reset,
  confirming UART is live before any events begin
- `status` command: all three counters (uptime, events_total,
  dropped) reported accurate values at every test point
- Folder structure matches task4/submission/ requirement exactly
- main.c contains zero raw register access — pure application logic

---

### What Limitations Remain

#### Hardware Pin Conflict: PD6 LED vs UART RX
The CH32V003F4U6's default UART RX pin (PD6) is the same physical
pin as the onboard user LED. When UART RX is enabled (as required
for command input), PD6 is owned by USART1 and cannot simultaneously
drive the LED.
**Workaround applied:** An external LED with 220Ω series resistor
was connected to PC0 and used as the application LED throughout
this project.
**Impact:** The onboard LED does not light up during this demo.
This is a hardware limitation of the CH32V003, not a firmware bug.

#### Command Length Cap
The UART command buffer is fixed at 23 usable characters
(EVENTQ_CMD_LEN - 1). Any input longer than this is silently
truncated. The terminal gives no feedback that truncation occurred.
In practice, all five supported commands are well within this limit,
so this does not affect the demo.

#### No Persistent State
All application state (uptime, event counters, LED state, dropped
count) lives in RAM and is reset to zero on every power cycle or
reset. There is no flash write-back in this project.
This is acceptable for a demo application but would need to be
addressed in a production system.

#### Single-Threaded Cooperative Model
The event queue and dispatcher run in a single tight loop with
no preemption. A handler that blocks (for example, a long uart_print
sequence) delays all other event processing during that time.
In this project, all handlers complete in under 1ms, so this is
not observable. A production system with longer handlers would
need either a mini-RTOS (Project-8 scope) or handler time budgets.

#### Queue Capacity Fixed at 16
If more than 16 events are produced before the dispatcher can
drain them, new events are dropped and counted in g_events_dropped.
Under the tested workload (1 tick/second + occasional button +
occasional command) the queue never exceeded depth 2.
A higher-frequency event source (e.g., ADC sampling at 100Hz)
would require increasing EVENTQ_SIZE.

#### No Echo on UART Input
Characters typed in the terminal are not echoed back to the screen.
The user sees nothing until they press Enter, at which point the
full `[CMD ]` log line appears. This is functional but not ideal
for usability. Adding echo would require one extra uart_print(c)
call in producer_uart(), which was omitted to keep the code clean.