# API Reference

This document describes every public function in the GPIO and UART driver libraries. The format mirrors Doxygen-style documentation as used in professional embedded SDK releases.

---

## GPIO Driver (`gpio.h` / `gpio.c`)

### Overview

The GPIO driver wraps direct register access for GPIOA, GPIOC, and GPIOD on the CH32V003. It handles APB2 clock gating, pin direction configuration, digital read/write, and a software debounce filter for mechanical inputs.

All functions accept a `GPIO_TypeDef *port` pointer. Use the port aliases defined in `gpio.h` rather than the raw HAL names.

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

Returned by `gpio_debounce_read()` when the pin is still bouncing. The caller must not treat this as a valid logic level.

#### Onboard Pin Aliases

```c
#define LED_PIN  6   // PD6 — onboard green LED
#define BTN_PIN  4   // PD4 — onboard tactile button
```

---

### `gpio_init`

```c
void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode);
```

**Purpose:** Enable the APB2 peripheral clock for the given port and configure one pin's direction and drive mode. Must be called before any other GPIO function on that pin.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `port` | `GPIO_TypeDef *` | Target port. Use `PORT_A`, `PORT_C`, or `PORT_D`. |
| `pin` | `uint8_t` | Pin number within the port, 0–7. |
| `mode` | `uint8_t` | One of `GPIO_OUTPUT`, `GPIO_INPUT`, `GPIO_INPUT_PU`, `GPIO_INPUT_PD`. |

**Return value:** None.

**Constraints:**
- On CH32V003, only GPIOA, GPIOC, and GPIOD exist. Passing any other port produces undefined behaviour.
- Pin numbers outside 0–7 will corrupt adjacent bits in `CFGLR` due to the 4-bit-per-pin shift calculation.
- For `GPIO_INPUT_PU`, the function writes the pin's bit in `OUTDR` to 1 after setting the mode bits. This is how the CH32V003 activates its internal pull-up: the `OUTDR` bit selects pull direction when `CNF[1:0] = 10b`.

**How it works internally:**

The CH32V003 `CFGLR` register allocates 4 bits per pin (bits `[4n+3 : 4n]`). The lower 2 bits (`MODE[1:0]`) set the direction; `00b` means input, and any non-zero value means output with a speed setting. The upper 2 bits (`CNF[1:0]`) refine the mode. For outputs, `00b` is push-pull. For inputs, `01b` is floating, `10b` activates the pull resistor (direction selected by `OUTDR`).

`gpio_init` clears all 4 bits first, then OR-writes the correct nibble for the requested mode:

| Mode constant | `CFGLR` nibble | Additional step |
|---|---|---|
| `GPIO_OUTPUT` | `0x3` (MODE=11, CNF=00) | — |
| `GPIO_INPUT` | `0x4` (MODE=00, CNF=01) | — |
| `GPIO_INPUT_PU` | `0x8` (MODE=00, CNF=10) | `OUTDR` bit set to 1 |
| `GPIO_INPUT_PD` | `0x8` (MODE=00, CNF=10) | `OUTDR` bit cleared to 0 |

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
void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value);
```

**Purpose:** Drive an output pin to a known logic level (HIGH or LOW). Has no effect on pins configured as inputs.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `port` | `GPIO_TypeDef *` | Target port. |
| `pin` | `uint8_t` | Pin number, 0–7. |
| `value` | `uint8_t` | `GPIO_HIGH` (1) to drive the pin high, `GPIO_LOW` (0) to drive it low. |

**Return value:** None.

**How it works internally:** The function sets or clears the corresponding bit in `OUTDR`. Writing 1 to a bit drives the push-pull output high; writing 0 drives it low. This is a read-modify-write on `OUTDR` using bitwise OR or AND-NOT.

**Note:** On the CH32V003, the `BSHR` (bit set/reset) register provides an atomic alternative for setting or clearing individual bits. This driver uses `OUTDR` for simplicity; in interrupt-driven code where a read-modify-write race is a concern, prefer `BSHR`.

**Example:**

```c
gpio_write(PORT_D, 6, GPIO_HIGH);   // LED on
gpio_write(PORT_D, 6, GPIO_LOW);    // LED off
```

---

### `gpio_toggle`

```c
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);
```

**Purpose:** Invert the current output level of a pin without needing to know its current state.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `port` | `GPIO_TypeDef *` | Target port. |
| `pin` | `uint8_t` | Pin number, 0–7. |

**Return value:** None.

**How it works internally:** XOR of the target bit in `OUTDR` with 1 flips it. If it was 1 it becomes 0; if it was 0 it becomes 1.

**Example:**

```c
// Blink LED every loop iteration
gpio_toggle(PORT_D, 6);
```

---

### `gpio_read`

```c
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);
```

**Purpose:** Sample the live logic level on a pin and return it as 0 or 1. Works on both input-configured and output-configured pins. On output pins it reads back the driven level.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `port` | `GPIO_TypeDef *` | Target port. |
| `pin` | `uint8_t` | Pin number, 0–7. |

**Return value:** `GPIO_HIGH` (1) if the pin is at a high voltage, `GPIO_LOW` (0) if low. Always 0 or 1; no other values are possible.

**How it works internally:** Reads `INDR` (input data register), shifts right by `pin` positions, masks with `0x1`. The `INDR` register captures the voltage on every pin of the port at each APB2 clock cycle. It reflects the actual electrical state, not the value last written to `OUTDR`.

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
uint8_t gpio_debounce_read(GPIO_TypeDef *port, uint8_t pin, uint8_t samples);
```

**Purpose:** Read a pin multiple times in succession and return the confirmed stable level. If any two consecutive samples disagree, the pin is still bouncing and the function returns `GPIO_DEBOUNCE_UNSTABLE` to signal the caller to retry later.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `port` | `GPIO_TypeDef *` | Target port. |
| `pin` | `uint8_t` | Pin number, 0–7. |
| `samples` | `uint8_t` | Number of samples to take. Must be ≥ 2. The demo uses 5. |

**Return value:**
- `GPIO_HIGH` (1) — pin is stably high across all samples.
- `GPIO_LOW` (0) — pin is stably low across all samples.
- `GPIO_DEBOUNCE_UNSTABLE` (0xFF) — level changed between samples; caller must discard this reading and retry.

**Timing:** Between each sample, the function runs a busy-wait loop of 1000 iterations. At 24 MHz with typical loop overhead, this is approximately 40–42 µs per gap. With 5 samples and 4 gaps, the total call takes roughly 160–170 µs.

**How it works internally:** The first sample is taken and stored as `ref`. Each subsequent sample is compared to `ref`. If any sample differs from `ref`, the function immediately returns `GPIO_DEBOUNCE_UNSTABLE`. If all `samples` agree, `ref` is returned.

**Why this approach:** Mechanical button contacts bounce for 1–20 ms after a state change. A single `gpio_read()` called in a tight loop can see dozens of false transitions during this window. By requiring all samples within the ~160 µs window to agree before reporting a result, and by returning `GPIO_DEBOUNCE_UNSTABLE` during the bounce window so the caller does nothing, the application receives exactly one logical event per physical press.

**Constraints:**
- `samples` = 1 is equivalent to a plain `gpio_read()` with no debounce benefit.
- The busy-wait delays are blocking. Do not call this function from an interrupt context or from latency-sensitive code.
- The debounce window (~160 µs for 5 samples) is much shorter than typical bounce duration (~5 ms). The caller is expected to call this function in a polling loop and discard `GPIO_DEBOUNCE_UNSTABLE` returns until the pin settles.

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

The UART driver configures USART1 on the CH32V003 for TX-only, 8N1 serial output. It provides blocking print functions suitable for debug logging. RX is not enabled in this driver version.

TX is fixed to **PD5** (default USART1 mapping on CH32V003). The baud rate is calculated from the 24 MHz `SystemCoreClock` constant.

---

### `uart_init`

```c
void uart_init(uint32_t baud);
```

**Purpose:** Enable the USART1 and GPIOD APB2 clocks, configure PD5 as an alternate-function push-pull output, set the baud rate divisor, and start the transmitter.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `baud` | `uint32_t` | Desired baud rate in bits per second. Common value: `115200`. |

**Return value:** None.

**How it works internally:** The baud rate register `BRR` is set to `SystemCoreClock / baud`. At 24 MHz and 115200 baud this gives `BRR = 208`, which produces an actual baud rate of 115384 bps — an error of 0.16%, within the USART module's 3% tolerance. PD5 is configured with `CFGLR` nibble `0xB` (MODE=11 = 50 MHz output, CNF=10 = alternate function push-pull). `CTLR1` is set with `UE` (USART enable) and `TE` (transmitter enable) bits.

**Constraints:** Must be called before any `uart_print*` function. `SystemCoreClock` must be 24 MHz for the baud rate calculation to be accurate.

**Example:**

```c
uart_init(115200);
```

---

### `uart_send_byte`

```c
void uart_send_byte(uint8_t byte);
```

**Purpose:** Transmit one byte over USART1. Blocks until the TX data register is empty before writing.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `byte` | `uint8_t` | The byte value to transmit. |

**Return value:** None.

**How it works internally:** Polls `STATR` bit 7 (`TXE` — transmit data register empty). When the shift register has accepted the previous byte, `TXE` sets and the function writes the new byte to `DATAR`. At 115200 baud, each byte takes approximately 87 µs.

**Example:**

```c
uart_send_byte('A');
```

---

### `uart_print`

```c
void uart_print(const char *str);
```

**Purpose:** Transmit a null-terminated string over USART1 without a trailing newline.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `str` | `const char *` | Pointer to a null-terminated ASCII string. |

**Return value:** None.

**Example:**

```c
uart_print("LED = ");
```

---

### `uart_println`

```c
void uart_println(const char *str);
```

**Purpose:** Transmit a null-terminated string followed by a carriage return (`\r`) and line feed (`\n`). Use this for line-terminated log messages.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `str` | `const char *` | Pointer to a null-terminated ASCII string. |

**Return value:** None.

**Example:**

```c
uart_println("[INIT] SysTick : 1 ms tick active");
```

---

### `uart_print_num`

```c
void uart_print_num(uint32_t n);
```

**Purpose:** Transmit an unsigned 32-bit integer as a decimal ASCII string (no leading zeros, no suffix).

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `n` | `uint32_t` | Value to print. Range 0 – 4294967295. |

**Return value:** None.

**How it works internally:** Builds digits into a local 11-byte buffer by repeatedly taking `n % 10` and dividing by 10. The digits are generated in reverse order (least significant first) and then transmitted in reverse to produce the correct decimal representation. The special case `n == 0` transmits the character `'0'` directly.

**Example:**

```c
uart_print("t = ");
uart_print_num(millis);
uart_println(" ms");
```

---

### `uart_print_int`

```c
void uart_print_int(int32_t n);
```

**Purpose:** Transmit a signed 32-bit integer as a decimal ASCII string. Negative values are preceded by a `-` character.

**Parameters:**

| Parameter | Type | Description |
|---|---|---|
| `n` | `int32_t` | Value to print. Range –2147483648 – 2147483647. |

**Return value:** None.

**Example:**

```c
uart_print_int(-42);   // prints: -42
```
