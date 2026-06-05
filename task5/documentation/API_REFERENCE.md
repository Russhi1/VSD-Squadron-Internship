# API Reference

This document describes every public function in the GPIO driver library,
and the supporting UART driver used for debug output. The format follows
Doxygen-style documentation as used in professional embedded SDK releases.

---

## GPIO Driver (`gpio.h` / `gpio.c`)

### Overview

The GPIO driver wraps direct register access for GPIOA, GPIOC, and GPIOD on
the CH32V003. It handles APB2 clock gating, pin direction configuration,
digital read/write, and a software debounce filter for mechanical inputs.

All functions accept a `GPIO_TypeDef *port` pointer. Use the port aliases
defined in `gpio.h` rather than the raw HAL names.

This is the primary driver for this project. The UART driver below is a
supporting utility for debug output only.

---

### Constants

#### Port Aliases

```c
#define PORT_A  GPIOA
#define PORT_C  GPIOC
#define PORT_D  GPIOD
```

These wrap the vendor HAL register structs into shorter names for readability.

#### Pin Mode Constants

```c
#define GPIO_OUTPUT    0   // Push-pull output, ~50 MHz slew
#define GPIO_INPUT     1   // Floating input — no pull resistor
#define GPIO_INPUT_PU  2   // Input with internal pull-up resistor
#define GPIO_INPUT_PD  3   // Input with internal pull-down resistor
```

#### Logic Level Constants

```c
#define GPIO_HIGH  1
#define GPIO_LOW   0
```

#### Special Return Value

```c
#define GPIO_DEBOUNCE_UNSTABLE  0xFF
```

Returned by `gpio_debounce_read()` when the pin is still bouncing.
The caller must not treat this as a valid logic level.

#### Onboard Pin Aliases

```c
#define LED_PIN  6   // PD6 — onboard green LED
#define BTN_PIN  4   // PD4 — onboard tactile button
```

---

### `gpio_init`

```c
/**
 * @brief  Configure a GPIO pin direction and drive mode.
 *
 * Enables the APB2 peripheral clock for the target port and
 * writes the correct 4-bit nibble into CFGLR. Must be called
 * before any other GPIO operation on this pin.
 *
 * @param  port   Target GPIO port. Use PORT_A, PORT_C, or PORT_D.
 * @param  pin    Pin number within the port (0–7).
 * @param  mode   GPIO_OUTPUT, GPIO_INPUT, GPIO_INPUT_PU, or GPIO_INPUT_PD.
 * @return None
 *
 * @note   Passing a pin number outside 0–7 will corrupt adjacent
 *         pin bits in CFGLR due to the 4-bit shift calculation.
 * @note   Only GPIOA, GPIOC, GPIOD exist on CH32V003. Any other
 *         port pointer produces undefined behaviour.
 * @note   For GPIO_INPUT_PU, OUTDR bit is set to 1 after mode bits
 *         are written. This is how CH32V003 activates its internal
 *         pull-up: the OUTDR bit selects pull direction when
 *         CNF[1:0] = 10b.
 */
void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode);
```

**Purpose:** Enable the APB2 peripheral clock for the given port and configure
one pin's direction and drive mode. Must be called before any other GPIO
function on that pin.

**Parameters:**

| Parameter | Type            | Description                                                      |
|-----------|-----------------|------------------------------------------------------------------|
| `port`    | `GPIO_TypeDef *`| Target port. Use `PORT_A`, `PORT_C`, or `PORT_D`.               |
| `pin`     | `uint8_t`       | Pin number within the port, 0–7.                                 |
| `mode`    | `uint8_t`       | One of `GPIO_OUTPUT`, `GPIO_INPUT`, `GPIO_INPUT_PU`, `GPIO_INPUT_PD`. |

**Return value:** None.

**Constraints:**
- On CH32V003, only GPIOA, GPIOC, and GPIOD exist. Passing any other port
  produces undefined behaviour.
- Pin numbers outside 0–7 will corrupt adjacent bits in `CFGLR` due to the
  4-bit-per-pin shift calculation.
- For `GPIO_INPUT_PU`, the function writes the pin's bit in `OUTDR` to 1
  after setting the mode bits. This is how the CH32V003 activates its internal
  pull-up: the `OUTDR` bit selects pull direction when `CNF[1:0] = 10b`.

**How it works internally:**

The CH32V003 `CFGLR` register allocates 4 bits per pin (bits `[4n+3 : 4n]`).
The lower 2 bits (`MODE[1:0]`) set the direction; `00b` means input, and any
non-zero value means output with a speed setting. The upper 2 bits
(`CNF[1:0]`) refine the mode.

`gpio_init` clears all 4 bits first, then OR-writes the correct nibble:

| Mode constant  | `CFGLR` nibble            | Additional step          |
|----------------|---------------------------|--------------------------|
| `GPIO_OUTPUT`  | `0x3` (MODE=11, CNF=00)   | —                        |
| `GPIO_INPUT`   | `0x4` (MODE=00, CNF=01)   | —                        |
| `GPIO_INPUT_PU`| `0x8` (MODE=00, CNF=10)   | `OUTDR` bit set to 1     |
| `GPIO_INPUT_PD`| `0x8` (MODE=00, CNF=10)   | `OUTDR` bit cleared to 0 |

**Example:**

```c
// Configure PD6 as output (LED)
gpio_init(PORT_D, 6, GPIO_OUTPUT);

// Configure PD4 as input with pull-up (button)
gpio_init(PORT_D, 4, GPIO_INPUT_PU);
```

---

### `gpio_write`

```c
/**
 * @brief  Drive an output pin to a known logic level.
 *
 * Sets or clears the corresponding bit in OUTDR. Has no
 * effect on pins configured as inputs.
 *
 * @param  port   Target GPIO port. Use PORT_A, PORT_C, or PORT_D.
 * @param  pin    Pin number within the port (0–7).
 * @param  value  GPIO_HIGH to drive high, GPIO_LOW to drive low.
 * @return None
 *
 * @note   Uses read-modify-write on OUTDR. In interrupt-driven
 *         code, prefer BSHR for atomic bit manipulation.
 */
void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value);
```

**Purpose:** Drive an output pin to a known logic level (HIGH or LOW).
Has no effect on pins configured as inputs.

**Parameters:**

| Parameter | Type            | Description                                          |
|-----------|-----------------|------------------------------------------------------|
| `port`    | `GPIO_TypeDef *`| Target port.                                         |
| `pin`     | `uint8_t`       | Pin number, 0–7.                                     |
| `value`   | `uint8_t`       | `GPIO_HIGH` (1) to drive high, `GPIO_LOW` (0) to drive low. |

**Return value:** None.

**How it works internally:** Sets or clears the corresponding bit in `OUTDR`
using bitwise OR or AND-NOT. Writing 1 drives the push-pull output high;
writing 0 drives it low.

**Constraints:**
- This is a read-modify-write on `OUTDR`. In interrupt-driven code where a
  race condition is a concern, use the `BSHR` register instead for atomic
  bit manipulation.

**Example:**

```c
gpio_write(PORT_D, 6, GPIO_HIGH);   // LED on
gpio_write(PORT_D, 6, GPIO_LOW);    // LED off
```

---

### `gpio_toggle`

```c
/**
 * @brief  Invert the current output level of a pin.
 *
 * XOR-flips the target bit in OUTDR without requiring
 * knowledge of the current pin state.
 *
 * @param  port   Target GPIO port. Use PORT_A, PORT_C, or PORT_D.
 * @param  pin    Pin number within the port (0–7).
 * @return None
 */
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);
```

**Purpose:** Invert the current output level of a pin without needing to
know its current state.

**Parameters:**

| Parameter | Type            | Description            |
|-----------|-----------------|------------------------|
| `port`    | `GPIO_TypeDef *`| Target port.           |
| `pin`     | `uint8_t`       | Pin number, 0–7.       |

**Return value:** None.

**How it works internally:** XOR of the target bit in `OUTDR` with 1 flips
it. If it was 1 it becomes 0; if it was 0 it becomes 1.

**Example:**

```c
// Blink LED every loop iteration
gpio_toggle(PORT_D, 6);
```

---

### `gpio_read`

```c
/**
 * @brief  Sample the live logic level on a pin.
 *
 * Reads the INDR (input data register) which reflects the
 * actual electrical state of the pin regardless of OUTDR.
 * Works on both input-configured and output-configured pins.
 *
 * @param  port   Target GPIO port. Use PORT_A, PORT_C, or PORT_D.
 * @param  pin    Pin number within the port (0–7).
 * @return GPIO_HIGH (1) if pin voltage is high.
 *         GPIO_LOW  (0) if pin voltage is low.
 *         Always returns exactly 0 or 1, no other values possible.
 */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);
```

**Purpose:** Sample the live logic level on a pin and return it as 0 or 1.
Works on both input-configured and output-configured pins. On output pins
it reads back the driven level.

**Parameters:**

| Parameter | Type            | Description      |
|-----------|-----------------|------------------|
| `port`    | `GPIO_TypeDef *`| Target port.     |
| `pin`     | `uint8_t`       | Pin number, 0–7. |

**Return value:** `GPIO_HIGH` (1) if the pin is at a high voltage,
`GPIO_LOW` (0) if low. Always 0 or 1; no other values are possible.

**How it works internally:** Reads `INDR` (input data register), shifts
right by `pin` positions, masks with `0x1`. The `INDR` register captures
the voltage on every pin of the port at each APB2 clock cycle. It reflects
the actual electrical state, not the value last written to `OUTDR`.

**Example:**

```c
uint8_t level = gpio_read(PORT_D, 4);
if (level == GPIO_LOW) {
    // Button is pressed (active-low with pull-up)
}
```

---

### `gpio_debounce_read`

```c
/**
 * @brief  Read a pin with a software debounce filter.
 *
 * Takes 'samples' readings with ~40 us between each. If all
 * samples agree, returns the stable level. If any two samples
 * differ, returns GPIO_DEBOUNCE_UNSTABLE to signal the caller
 * to retry on the next loop iteration.
 *
 * @param  port     Target GPIO port. Use PORT_A, PORT_C, or PORT_D.
 * @param  pin      Pin number within the port (0–7).
 * @param  samples  Number of samples to take. Must be >= 2.
 *                  Passing 0 returns an unsampled value — do not use.
 *                  Passing 1 is equivalent to a plain gpio_read()
 *                  with no debounce benefit.
 * @return GPIO_HIGH (1)                 — pin stably high.
 *         GPIO_LOW  (0)                 — pin stably low.
 *         GPIO_DEBOUNCE_UNSTABLE (0xFF) — pin still bouncing; retry.
 *
 * @note   Blocking call. Do not call from an interrupt context or
 *         from latency-sensitive code.
 * @note   Total blocking time ≈ (samples - 1) × 40 us.
 *         With samples = 5, this is approximately 160 us.
 * @note   The caller must check for GPIO_DEBOUNCE_UNSTABLE before
 *         treating the return value as a logic level.
 */
uint8_t gpio_debounce_read(GPIO_TypeDef *port, uint8_t pin, uint8_t samples);
```

**Purpose:** Read a pin multiple times in succession and return the confirmed
stable level. If any two consecutive samples disagree, the pin is still
bouncing and the function returns `GPIO_DEBOUNCE_UNSTABLE` to signal the
caller to retry later.

**Parameters:**

| Parameter | Type            | Description                                                  |
|-----------|-----------------|--------------------------------------------------------------|
| `port`    | `GPIO_TypeDef *`| Target port.                                                 |
| `pin`     | `uint8_t`       | Pin number, 0–7.                                             |
| `samples` | `uint8_t`       | Number of samples to take. Must be ≥ 2. The demo uses 5.    |

**Return value:**
- `GPIO_HIGH` (1) — pin is stably high across all samples.
- `GPIO_LOW` (0) — pin is stably low across all samples.
- `GPIO_DEBOUNCE_UNSTABLE` (0xFF) — level changed between samples;
  caller must discard this reading and retry.

**Timing:** Between each sample, the function runs a busy-wait loop of
1000 iterations. At 24 MHz with typical loop overhead, this is approximately
40–42 µs per gap. With 5 samples and 4 gaps, the total call takes roughly
160–170 µs.

**How it works internally:** The first sample is taken and stored as `ref`.
Each subsequent sample is compared to `ref`. If any sample differs from
`ref`, the function immediately returns `GPIO_DEBOUNCE_UNSTABLE`. If all
`samples` agree, `ref` is returned.

**Why this approach:** Mechanical button contacts bounce for 1–20 ms after
a state change. A single `gpio_read()` in a tight loop can see dozens of
false transitions during this window. By requiring all samples within the
~160 µs window to agree, and returning `GPIO_DEBOUNCE_UNSTABLE` during the
bounce window, the application receives exactly one logical event per
physical press.

**Constraints:**
- `samples` = 1 is equivalent to a plain `gpio_read()` with no debounce benefit.
- `samples` = 0 returns an unsampled value — do not use.
- The busy-wait delays are blocking. Do not call from an interrupt context
  or from latency-sensitive code.
- The debounce window (~160 µs for 5 samples) is much shorter than typical
  bounce duration (~5 ms). The caller is expected to call this function in a
  polling loop and discard `GPIO_DEBOUNCE_UNSTABLE` returns until the pin settles.

**Example:**

```c
uint8_t btn = gpio_debounce_read(PORT_D, BTN_PIN, 5);
if (btn == GPIO_DEBOUNCE_UNSTABLE) {
    // Pin is still bouncing — skip this iteration
} else if (btn == GPIO_LOW) {
    // Stable press detected
}
```

---

## UART Driver (`uart.h` / `uart.c`)

### Overview

The UART driver is a supporting utility used exclusively for debug output in
this project. It configures USART1 on the CH32V003 for TX-only, 8N1 serial
output with blocking transmission.

**Limitations of this driver:**
- TX only — RX is not implemented.
- Blocking transmission — no interrupt-driven buffer, no DMA.
- Fixed to PD5 (default USART1 TX mapping on CH32V003).
- Baud rate accuracy assumes SystemCoreClock = 24 MHz.

These limitations are intentional. The UART is only used for human-readable
debug logging at 115200 baud, where blocking is acceptable.

---

### `uart_init`

```c
/**
 * @brief  Configure USART1 for TX-only serial output.
 *
 * Enables APB2 clocks for GPIOD and USART1, configures PD5 as
 * alternate-function push-pull output, sets the baud rate divisor,
 * and enables the transmitter.
 *
 * @param  baud  Desired baud rate in bits per second (e.g. 115200).
 * @return None
 *
 * @note   RX is not enabled. This driver is TX-only.
 * @note   Assumes SystemCoreClock = 24 MHz.
 * @note   Must be called before any uart_print* function.
 */
void uart_init(uint32_t baud);
```

**Parameters:**

| Parameter | Type       | Description                              |
|-----------|------------|------------------------------------------|
| `baud`    | `uint32_t` | Desired baud rate in bits per second.    |

**Return value:** None.

**Example:**

```c
uart_init(115200);
```

---

### `uart_send_byte`

```c
/**
 * @brief  Transmit one byte over USART1.
 *
 * Blocks until the TX data register is empty (TXE flag set),
 * then writes the byte to DATAR.
 *
 * @param  byte  The byte value to transmit.
 * @return None
 *
 * @note   Blocking. At 115200 baud each byte takes ~87 us.
 */
void uart_send_byte(uint8_t byte);
```

**Parameters:**

| Parameter | Type      | Description              |
|-----------|-----------|--------------------------|
| `byte`    | `uint8_t` | The byte value to transmit. |

**Return value:** None.

**Example:**

```c
uart_send_byte('A');
```

---

### `uart_print`

```c
/**
 * @brief  Transmit a null-terminated string over USART1.
 *
 * @param  str  Pointer to a null-terminated ASCII string.
 * @return None
 */
void uart_print(const char *str);
```

**Example:**

```c
uart_print("LED = ");
```

---

### `uart_println`

```c
/**
 * @brief  Transmit a null-terminated string followed by CR+LF.
 *
 * @param  str  Pointer to a null-terminated ASCII string.
 * @return None
 */
void uart_println(const char *str);
```

**Example:**

```c
uart_println("[INIT] SysTick : 1 ms tick active");
```

---

### `uart_print_num`

```c
/**
 * @brief  Transmit an unsigned 32-bit integer as decimal ASCII.
 *
 * No leading zeros. No suffix. Special case: n = 0 prints "0".
 *
 * @param  n  Value to transmit. Range: 0 to 4294967295.
 * @return None
 */
void uart_print_num(uint32_t n);
```

**Example:**

```c
uart_print("t = ");
uart_print_num(timer_get_millis());
uart_println(" ms");
```

---

### `uart_print_int`

```c
/**
 * @brief  Transmit a signed 32-bit integer as decimal ASCII.
 *
 * Negative values are preceded by a '-' character.
 *
 * @param  n  Value to transmit. Range: -2147483648 to 2147483647.
 * @return None
 */
void uart_print_int(int32_t n);
```

**Example:**

```c
uart_print_int(-42);   // prints: -42
```