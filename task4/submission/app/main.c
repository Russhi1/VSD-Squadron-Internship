

#include "gpio.h"
#include "timer.h"
#include "uart.h"
#include "eventq.h"

/* ── Timing thresholds ──────────────────────────────────────────────── */
#define LONG_PRESS_MS    500u
#define DOUBLE_TAP_MS    400u
#define DEBOUNCE_MS       20u

/* ── LED blink parameters per event type ────────────────────────────── */
#define SHORT_ON_MS      200u
#define LONG_ON_MS      2000u
#define RAPID_ON_MS       80u
#define RAPID_OFF_MS      80u
#define RAPID_COUNT        5u

/* ── Shared event queue ──────────────────────────────────────────────── */
static EventQueue g_queue;

/* ── Statistics ─────────────────────────────────────────────────────── */
static uint32_t g_total_events   = 0u;
static uint32_t g_dropped_events = 0u;

/* ─────────────────────────────────────────────────────────────────────
 * BUTTON PRODUCER
 *
 * The EXTI ISR records the raw edge time and direction into two volatile
 * variables.  The producer state machine (polled each main-loop iteration)
 * reads them and classifies the press gesture.
 *
 * State machine:
 *
 *   IDLE
 *     waiting for first falling edge (button pressed)
 *     → on falling edge: record press_start, go to PRESSED
 *
 *   PRESSED
 *     button is currently held down
 *     → on rising edge (release): record release_time
 *       • if hold >= LONG_PRESS_MS: push EVT_LONG_PRESS, go to IDLE
 *       • else: go to WAIT_SECOND (counting from release_time)
 *
 *   WAIT_SECOND
 *     waiting to see if a second tap arrives within DOUBLE_TAP_MS
 *     → on falling edge (second press): push EVT_DOUBLE_TAP, go to CONSUME_SECOND
 *     → on timeout (DOUBLE_TAP_MS expired): push EVT_SHORT_PRESS, go to IDLE
 *
 *   CONSUME_SECOND
 *     absorb the second press release so we don't produce a third event
 *     → on rising edge: go to IDLE
 * ───────────────────────────────────────────────────────────────────── */

typedef enum {
    BTN_IDLE,
    BTN_PRESSED,
    BTN_WAIT_SECOND,
    BTN_CONSUME_SECOND,
} BtnState;

/*
 * Raw edge data written by EXTI ISR and read by the producer.
 * volatile prevents compiler from caching these in registers.
 */
volatile uint8_t  g_edge_pending   = 0u;  /* 1 when a new edge arrived   */
volatile uint8_t  g_edge_level     = 1u;  /* GPIO level AFTER the edge   */
volatile uint32_t g_edge_timestamp = 0u;  /* timer_get_millis() at edge  */

static void button_producer(void)
{
    static BtnState state       = BTN_IDLE;
    static uint32_t press_start = 0u;
    static uint32_t release_t   = 0u;
    /* Snapshot current raw level once for debounce */
    static uint8_t  last_raw    = 1u;
    static uint32_t stable_t    = 0u;
    static uint8_t  confirmed   = 1u;

    uint32_t now = timer_get_millis();

    /*
     * Debounce: sample the pin every call; if the raw level has been
     * stable for DEBOUNCE_MS, update confirmed and act on the edge.
     */
    uint8_t raw = gpio_read(BTN_PORT, BTN_PIN);
    if (raw != last_raw) {
        last_raw = raw;
        stable_t = now;
    }

    if ((now - stable_t) >= DEBOUNCE_MS) {
        if (raw != confirmed) {
            /* A real edge has settled */
            confirmed = raw;

            switch (state) {

            case BTN_IDLE:
                if (confirmed == GPIO_LOW) {
                    /* Falling edge: button pressed */
                    press_start = now;
                    state = BTN_PRESSED;
                }
                break;

            case BTN_PRESSED:
                if (confirmed == GPIO_HIGH) {
                    /* Rising edge: button released */
                    release_t = now;
                    uint32_t hold = release_t - press_start;
                    if (hold >= LONG_PRESS_MS) {
                        /* Held long enough — classify immediately */
                        if (!eventq_push(&g_queue, EVT_LONG_PRESS, release_t)) {
                            g_dropped_events++;
                        }
                        state = BTN_IDLE;
                    } else {
                        /* Short release — wait to see if second tap follows */
                        state = BTN_WAIT_SECOND;
                    }
                }
                break;

            case BTN_WAIT_SECOND:
                if (confirmed == GPIO_LOW) {
                    /* Second press arrived in time */
                    if (!eventq_push(&g_queue, EVT_DOUBLE_TAP, now)) {
                        g_dropped_events++;
                    }
                    state = BTN_CONSUME_SECOND;
                }
                break;

            case BTN_CONSUME_SECOND:
                if (confirmed == GPIO_HIGH) {
                    /* Absorb the release of the second tap */
                    state = BTN_IDLE;
                }
                break;

            default:
                state = BTN_IDLE;
                break;
            }
        }
    }

    /*
     * Timeout check for WAIT_SECOND: if DOUBLE_TAP_MS has expired
     * without a second press, emit a short press and return to IDLE.
     */
    if (state == BTN_WAIT_SECOND) {
        if ((now - release_t) >= DOUBLE_TAP_MS) {
            if (!eventq_push(&g_queue, EVT_SHORT_PRESS, release_t)) {
                g_dropped_events++;
            }
            state = BTN_IDLE;
        }
    }
}

/* ─────────────────────────────────────────────────────────────────────
 * LED UTILITY
 * Blocking delay loop using timer_get_millis().
 * ───────────────────────────────────────────────────────────────────── */
static void delay_ms(uint32_t ms)
{
    uint32_t start = timer_get_millis();
    while ((timer_get_millis() - start) < ms);
}

/* ─────────────────────────────────────────────────────────────────────
 * EVENT HANDLERS
 * Each handler is responsible for:
 *   1. Driving the LED to produce the gesture-specific pattern.
 *   2. Printing a UART log line.
 * Handlers never touch the event queue or the button state.
 * ───────────────────────────────────────────────────────────────────── */

static void handler_short_press(const Event *e)
{
    uart_print("[DISPATCH] EVT_SHORT_PRESS  t=");
    uart_print_num(e->timestamp);
    uart_println("ms  -> single blink 200ms");

    gpio_write(LED_PORT, LED_PIN, GPIO_HIGH);
    delay_ms(SHORT_ON_MS);
    gpio_write(LED_PORT, LED_PIN, GPIO_LOW);
}

static void handler_long_press(const Event *e)
{
    uart_print("[DISPATCH] EVT_LONG_PRESS   t=");
    uart_print_num(e->timestamp);
    uart_println("ms  -> LED ON for 2 s");

    gpio_write(LED_PORT, LED_PIN, GPIO_HIGH);
    delay_ms(LONG_ON_MS);
    gpio_write(LED_PORT, LED_PIN, GPIO_LOW);

    uart_println("[DISPATCH] EVT_LONG_PRESS   -> LED OFF");
}

static void handler_double_tap(const Event *e)
{
    uart_print("[DISPATCH] EVT_DOUBLE_TAP   t=");
    uart_print_num(e->timestamp);
    uart_println("ms  -> 5x rapid blink");

    for (uint8_t i = 0u; i < RAPID_COUNT; i++) {
        gpio_write(LED_PORT, LED_PIN, GPIO_HIGH);
        delay_ms(RAPID_ON_MS);
        gpio_write(LED_PORT, LED_PIN, GPIO_LOW);
        delay_ms(RAPID_OFF_MS);
    }
}

/* ─────────────────────────────────────────────────────────────────────
 * DISPATCHER
 * Pops one event per call and routes it to the correct handler.
 * Called once per main-loop iteration.
 * ───────────────────────────────────────────────────────────────────── */
static void dispatch_one(void)
{
    Event e;
    if (!eventq_pop(&g_queue, &e)) {
        return;   /* queue empty this iteration */
    }

    g_total_events++;

    switch (e.type) {
        case EVT_SHORT_PRESS:  handler_short_press(&e); break;
        case EVT_LONG_PRESS:   handler_long_press(&e);  break;
        case EVT_DOUBLE_TAP:   handler_double_tap(&e);  break;
        default:
            uart_println("[DISPATCH] WARNING: unknown event type");
            break;
    }
}

/* ─────────────────────────────────────────────────────────────────────
 * STATUS PRINTER
 * Triggered when 's' is received over UART.
 * ───────────────────────────────────────────────────────────────────── */
static void print_status(void)
{
    uart_print("[STATUS]   total_dispatched=");
    uart_print_num(g_total_events);
    uart_print("  queue_depth=");
    uart_print_num(eventq_depth(&g_queue));
    uart_print("/");
    uart_print_num(EVENTQ_SIZE);
    uart_print("  dropped=");
    uart_print_num(g_dropped_events);
    uart_println("");
}


static void handle_uart_input(void)
{
    while (uart_rx_available()) {
        char c = uart_read_byte();
        switch (c) {
            case 's': case 'S':
                print_status();
                break;
            case 'h': case 'H':
                uart_println("[HELP]     s = status,  h = help");
                uart_println("[HELP]     Press button: short/long/double-tap");
                break;
            default:
                /* ignore other characters */
                break;
        }
    }
}

/* ─────────────────────────────────────────────────────────────────────
 * MAIN
 * ───────────────────────────────────────────────────────────────────── */
int main(void)
{
    /* ── Hardware init ── */
    gpio_init(LED_PORT, LED_PIN,  GPIO_MODE_OUTPUT);
    gpio_init(BTN_PORT, BTN_PIN,  GPIO_MODE_INPUT_PU);
    uart_init(115200u);
    timer_init();

    /* ── Framework init ── */
    eventq_init(&g_queue);

    /* ── LED off on startup ── */
    gpio_write(LED_PORT, LED_PIN, GPIO_LOW);

    /* ── Startup banner ── */
    uart_println("========================================");
    uart_println("  Smart Button Controller");
    uart_println("  VSDSquadron Mini — CH32V003");
    uart_println("========================================");
    uart_println("[INFO]  SHORT press  -> single blink");
    uart_println("[INFO]  LONG  press  -> LED ON 2 s");
    uart_println("[INFO]  DOUBLE tap   -> 5x rapid blink");
    uart_println("[INFO]  Type 'h' for help, 's' for status");
    uart_println("----------------------------------------");

    /* ── Main loop ── */
    while (1) {
        button_producer();   /* Sample button; push event when gesture complete */
        handle_uart_input(); /* Drain UART RX; respond to 's', 'h'              */
        dispatch_one();      /* Pop one event (if any); call its handler         */
    }

    return 0;
}