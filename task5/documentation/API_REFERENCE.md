# API Reference

Doxygen-style documentation for the GPIO driver (primary) and UART driver
(debug output only).

---

## GPIO Driver (`gpio.h` / `gpio.c`)

### Overview

Wraps direct register access for GPIOA, GPIOC, and GPIOD on the CH32V003.
Handles APB2 clock gating, pin direction, digital read/write, toggle, and
software debounce. All functions take a `GPIO_TypeDef *port` pointer — use
the port aliases in `gpio.h`.

### Constants

```c
/* Port aliases */
#define PORT_A  GPIOA
#define PORT_C  GPIOC
#define PORT_D  GPIOD

/* Pin modes */
#define GPIO_OUTPUT    0   // Push-pull output, ~50 MHz slew
#define GPIO_INPUT     1   // Floating input
#define GPIO_INPUT_PU  2   // Input with internal pull-up
#define GPIO_INPUT_PD  3   // Input with internal pull-down

/* Logic levels */
#define GPIO_HIGH  1
#define GPIO_LOW   0

/* Debounce return value — not a valid logic level */
#define GPIO_DEBOUNCE_UNSTABLE  0xFF

/* Onboard pin aliases */
#define LED_PIN  6   // PD6 — onboard green LED
#define BTN_PIN  4   // PD4 — onboard tactile button
```

---

### `gpio_init`

```c
/**
 * @brief  Configure a GPIO pin direction and drive mode.
 * @param  port   Target port — PORT_A, PORT_C, or PORT_D.
 * @param  pin    Pin number 0–7.
 * @param  mode   GPIO_OUTPUT | GPIO_INPUT | GPIO_INPUT_PU | GPIO_INPUT_PD
 * @return None
 * @note   Enables APB2 clock for the port internally.
 * @note   Pin numbers outside 0–7 corrupt adjacent CFGLR bits.
 * @note   GPIO_INPUT_PU sets OUTDR bit to 1 after writing mode bits —
 *         this is how CH32V003 activates pull-up when CNF[1:0] = 10b.
 */
void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode);
```

**Parameters:**

| Parameter | Type             | Description                          |
|-----------|------------------|--------------------------------------|
| `port`    | `GPIO_TypeDef *` | Target port. Use PORT_A/C/D.         |
| `pin`     | `uint8_t`        | Pin number, 0–7.                     |
| `mode`    | `uint8_t`        | One of the four mode constants above.|

**CFGLR nibble written per mode:**

| Mode           | Nibble                  | Extra step               |
|----------------|-------------------------|--------------------------|
| `GPIO_OUTPUT`  | `0x3` (MODE=11, CNF=00) | —                        |
| `GPIO_INPUT`   | `0x4` (MODE=00, CNF=01) | —                        |
| `GPIO_INPUT_PU`| `0x8` (MODE=00, CNF=10) | OUTDR bit set to 1       |
| `GPIO_INPUT_PD`| `0x8` (MODE=00, CNF=10) | OUTDR bit cleared to 0   |

```c
gpio_init(PORT_D, 6, GPIO_OUTPUT);   // LED
gpio_init(PORT_D, 4, GPIO_INPUT_PU); // Button with pull-up
```

---

### `gpio_write`

```c
/**
 * @brief  Drive an output pin HIGH or LOW.
 * @param  port   Target port.
 * @param  pin    Pin number, 0–7.
 * @param  value  GPIO_HIGH or GPIO_LOW.
 * @return None
 * @note   Read-modify-write on OUTDR. Use BSHR for atomic access
 *         in interrupt-driven code.
 */
void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t value);
```

```c
gpio_write(PORT_D, 6, GPIO_HIGH);  // LED on
gpio_write(PORT_D, 6, GPIO_LOW);   // LED off
```

---

### `gpio_toggle`

```c
/**
 * @brief  XOR-flip an output pin without knowing its current state.
 * @param  port  Target port.
 * @param  pin   Pin number, 0–7.
 * @return None
 */
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin);
```

```c
gpio_toggle(PORT_D, 6); // Flip LED
```

---

### `gpio_read`

```c
/**
 * @brief  Read the live logic level from INDR.
 * @param  port  Target port.
 * @param  pin   Pin number, 0–7.
 * @return GPIO_HIGH (1) or GPIO_LOW (0). Always exactly 0 or 1.
 * @note   Reads INDR, not OUTDR — reflects actual pin voltage.
 */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin);
```

```c
if (gpio_read(PORT_D, 4) == GPIO_LOW) {
    // Button pressed (active-low with pull-up)
}
```

---

### `gpio_debounce_read`

```c
/**
 * @brief  Read a pin with software debounce.
 *
 * Takes N samples with ~40 us gaps. Returns stable level if all
 * samples agree, or GPIO_DEBOUNCE_UNSTABLE if any sample differs.
 *
 * @param  port     Target port.
 * @param  pin      Pin number, 0–7.
 * @param  samples  Sample count. Must be >= 2. Do not pass 0.
 * @return GPIO_HIGH | GPIO_LOW | GPIO_DEBOUNCE_UNSTABLE (0xFF)
 * @note   Blocking — do not call from interrupt context.
 * @note   Blocking time ≈ (samples - 1) × 40 us.
 *         5 samples = ~160 us.
 * @note   Caller must check for UNSTABLE before using return value.
 */
uint8_t gpio_debounce_read(GPIO_TypeDef *port, uint8_t pin, uint8_t samples);
```

```c
uint8_t btn = gpio_debounce_read(PORT_D, BTN_PIN, 5);
if (btn == GPIO_DEBOUNCE_UNSTABLE) continue; // still bouncing
if (btn == GPIO_LOW) { /* stable press */ }
```

---

## UART Driver (`uart.h` / `uart.c`)

### Overview

Supporting debug utility only. TX-only, blocking, fixed to PD5 (USART1
default). Not intended for bidirectional communication.

**Limitations:** RX not implemented. No DMA. No interrupt-driven buffer.
Assumes `SystemCoreClock = 24 MHz`.

---

### `uart_init`

```c
/**
 * @brief  Configure USART1 TX on PD5 at the given baud rate.
 * @param  baud  Baud rate in bps (e.g. 115200).
 * @return None
 * @note   BRR = SystemCoreClock / baud. At 24 MHz / 115200 = 0.16% error.
 * @note   Must be called before any uart_print* function.
 */
void uart_init(uint32_t baud);
```

```c
uart_init(115200);
```

---

### `uart_send_byte`

```c
/**
 * @brief  Transmit one byte. Blocks until TXE flag is set.
 * @param  byte  Byte to transmit.
 * @return None
 * @note   At 115200 baud each byte takes ~87 us.
 */
void uart_send_byte(uint8_t byte);
```

---

### `uart_print` / `uart_println`

```c
/**
 * @brief  Transmit a null-terminated string.
 * @param  str  ASCII string pointer.
 * @return None
 */
void uart_print(const char *str);

/**
 * @brief  Transmit a null-terminated string followed by CR+LF.
 * @param  str  ASCII string pointer.
 * @return None
 */
void uart_println(const char *str);
```

```c
uart_print("LED = ");
uart_println("ON");
```

---

### `uart_print_num` / `uart_print_int`

```c
/**
 * @brief  Transmit uint32_t as decimal ASCII. Range: 0–4294967295.
 * @note   Avoids printf() — saves ~2 KB code size on bare-metal GCC.
 */
void uart_print_num(uint32_t n);

/**
 * @brief  Transmit int32_t as decimal ASCII. Negative prefixed with '-'.
 */
void uart_print_int(int32_t n);
```

```c
uart_print("t = ");
uart_print_num(timer_get_millis());
uart_println(" ms");
```