/*
 * gpio.c — GPIO Hardware Abstraction Library Implementation
 * VSD Squadron Mini (CH32V003F4U6)
 *
 * Register reference (CH32V003 Reference Manual, Chapter 7):
 *
 *   RCC->APB2PCENR
 *       Peripheral clock enable register.
 *       Each GPIO port has a dedicated enable bit.
 *       Must be set before ANY register access to that port.
 *       Using |= (set only, never clear) makes repeated calls safe.
 *
 *   GPIOx->CFGLR
 *       Configuration register for pins 0–7.
 *       Each pin occupies 4 bits: [CNF1:CNF0 | MODE1:MODE0]
 *       Bit offset for pin N = (N * 4)
 *
 *       MODE bits (direction and speed):
 *         00 = input
 *         01 = output 10 MHz
 *         10 = output  2 MHz
 *         11 = output 50 MHz   ← we always use this for outputs
 *
 *       CNF bits (type) when in OUTPUT mode (MODE != 00):
 *         00 = general purpose push-pull    ← we use this
 *         01 = general purpose open-drain
 *         10 = alternate function push-pull
 *         11 = alternate function open-drain
 *
 *       CNF bits when in INPUT mode (MODE == 00):
 *         00 = analog input
 *         01 = floating input               ← GPIO_MODE_INPUT
 *         10 = input with pull-up/down      ← GPIO_MODE_INPUT_PU/_PD
 *         11 = reserved
 *
 *   GPIOx->OUTDR
 *       Output data register.
 *       Bit N controls pin N.
 *       For OUTPUT pins: sets the driven voltage level.
 *       For INPUT pins with CNF=10 (pull mode): 1=pull-up, 0=pull-down.
 *
 *   GPIOx->INDR
 *       Input data register (read-only).
 *       Bit N reflects the current voltage on pin N.
 *       Sampled every HCLK cycle by hardware.
 *
 * 4-bit CFGLR field values used in this file:
 *   0x3 = 0011b → MODE=11 (50 MHz output), CNF=00 (push-pull)
 *   0x4 = 0100b → MODE=00 (input),         CNF=01 (floating)
 *   0x8 = 1000b → MODE=00 (input),         CNF=10 (pull resistor)
 */

#include "gpio.h"

/* -----------------------------------------------------------------------
 * enable_clock  — internal helper
 *
 * The CH32V003 powers all GPIO ports OFF after reset to save energy.
 * Writing 1 to the corresponding bit in APB2PCENR powers the port on.
 * This must happen before any other register in that port is touched.
 *
 * Using |= (bitwise OR assign) ensures we never accidentally disable
 * a port that was already enabled by a previous gpio_init() call.
 * ---------------------------------------------------------------------- */
static void enable_clock(GPIO_TypeDef *port)
{
    if      (port == GPIOA) { RCC->APB2PCENR |= RCC_APB2Periph_GPIOA; }
    else if (port == GPIOC) { RCC->APB2PCENR |= RCC_APB2Periph_GPIOC; }
    else if (port == GPIOD) { RCC->APB2PCENR |= RCC_APB2Periph_GPIOD; }
    /*
     * Unknown port: do nothing rather than crash.
     * In a production system this would assert().
     */
}

/* -----------------------------------------------------------------------
 * gpio_init
 *
 * Step-by-step process:
 *   1. Enable the port clock (safe to call multiple times)
 *   2. Clear the 4 CFGLR bits for this pin (reset to floating input first)
 *      This is mandatory — you must clear before writing the new value,
 *      otherwise you would OR new bits on top of old ones.
 *   3. Write the 4-bit configuration value for the requested mode
 *   4. For pull modes only: set or clear the OUTDR bit to choose
 *      pull-up vs pull-down direction
 * ---------------------------------------------------------------------- */
void gpio_init(GPIO_TypeDef *port, uint8_t pin, uint8_t mode)
{
    uint32_t shift = (uint32_t)pin * 4U;   /* bit position in CFGLR */

    /* Step 1 — power the port on */
    enable_clock(port);

    /* Step 2 — clear the 4 config bits for this pin */
    port->CFGLR &= ~(0x0FU << shift);

    /* Step 3 — write the new configuration */
    switch (mode) {

        case GPIO_MODE_OUTPUT:
            /*
             * 0x3 = 0b0011
             * CNF[1:0] = 00  →  general purpose push-pull output
             * MODE[1:0] = 11 →  output mode, 50 MHz drive speed
             *
             * Push-pull means the pin actively drives both HIGH (P-MOS on)
             * and LOW (N-MOS on). This gives clean logic levels and is
             * the correct mode for driving LEDs, buzzers, etc.
             */
            port->CFGLR |= (0x03U << shift);
            break;

        case GPIO_MODE_INPUT:
            /*
             * 0x4 = 0b0100
             * CNF[1:0] = 01  →  floating input (no pull resistor)
             * MODE[1:0] = 00 →  input mode
             *
             * The pin floats at whatever voltage is externally applied.
             * Use this when you have a guaranteed external drive (e.g.,
             * sensor with its own pull-up/down, or another MCU output).
             * Do NOT use for buttons — a floating button reads noise.
             */
            port->CFGLR |= (0x04U << shift);
            break;

        case GPIO_MODE_INPUT_PU:
            /*
             * 0x8 = 0b1000
             * CNF[1:0] = 10  →  input with pull resistor enabled
             * MODE[1:0] = 00 →  input mode
             *
             * Then set OUTDR bit = 1 to select pull-UP direction.
             *
             * With pull-up: pin reads HIGH when nothing is connected.
             * Button connected to GND → reads LOW when pressed.
             * This is the correct mode for the onboard button (PD4).
             */
            port->CFGLR |= (0x08U << shift);
            port->OUTDR  |= (1U << pin);     /* OUTDR=1 → pull-up selected */
            break;

        case GPIO_MODE_INPUT_PD:
            /*
             * Same CFGLR value as INPUT_PU (0x8), but OUTDR bit = 0
             * selects pull-DOWN instead of pull-up.
             *
             * Pin reads LOW when nothing is connected.
             * Use for signals that default to LOW (e.g., active-high buttons
             * connected to VCC).
             */
            port->CFGLR |= (0x08U << shift);
            port->OUTDR  &= ~(1U << pin);    /* OUTDR=0 → pull-down selected */
            break;

        default:
            /*
             * Unknown mode — leave the pin in the reset state (floating
             * input, which is what the hardware initializes to).
             * Step 2 already cleared the bits, so we just do nothing here.
             */
            break;
    }
}

/* -----------------------------------------------------------------------
 * gpio_write
 *
 * The OUTDR register is the output data register.
 * Each bit corresponds directly to a pin number.
 *
 * To set a bit without disturbing other pins: use |=
 * To clear a bit without disturbing other pins: use &= ~
 *
 * We never write the full register (port->OUTDR = value) because that
 * would clobber settings for all other pins on the same port.
 * ---------------------------------------------------------------------- */
void gpio_write(GPIO_TypeDef *port, uint8_t pin, uint8_t level)
{
    if (level == GPIO_HIGH) {
        port->OUTDR |=  (1U << pin);   /* drive pin to 3.3 V */
    } else {
        port->OUTDR &= ~(1U << pin);   /* drive pin to 0 V   */
    }
}

/* -----------------------------------------------------------------------
 * gpio_toggle
 *
 * XOR (^) flips a single bit cleanly:
 *   0 ^ 1 = 1   (was LOW  → now HIGH)
 *   1 ^ 1 = 0   (was HIGH → now LOW)
 *
 * All other bits in OUTDR are XOR'd with 0, so they are unchanged.
 * This is more efficient than reading OUTDR, flipping, and writing back
 * because XOR is a single atomic read-modify-write operation.
 * ---------------------------------------------------------------------- */
void gpio_toggle(GPIO_TypeDef *port, uint8_t pin)
{
    port->OUTDR ^= (1U << pin);
}

/* -----------------------------------------------------------------------
 * gpio_read
 *
 * The INDR register holds the live sampled voltage of every pin.
 * Hardware refreshes it every HCLK cycle regardless of pin direction.
 *
 * Reading input pins: gives the externally applied voltage.
 * Reading output pins: gives the voltage actually present on the pin,
 *   which may differ from OUTDR if the pin is being externally driven
 *   (useful for detecting short circuits in safety-critical code).
 *
 * We shift right to bring pin N's bit to position 0, then mask with 0x1
 * so the return value is always exactly 0 or 1.
 * ---------------------------------------------------------------------- */
uint8_t gpio_read(GPIO_TypeDef *port, uint8_t pin)
{
    return (uint8_t)((port->INDR >> pin) & 0x01U);
}