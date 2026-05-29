

#include <ch32v00x.h>
#include "gpio.h"
#include "uart.h"
#include "timer.h"
#include "eventq.h"

/* ── Application LED is on PC0 (PD6 is taken by UART RX) ────────────── */
#define APP_LED_PORT   PORT_C
#define APP_LED_PIN    0

/* ── Debounce window in milliseconds ─────────────────────────────────── */
#define DEBOUNCE_MS    50

/* ── Global event queue ──────────────────────────────────────────────── */
static EventQueue g_queue;

/* ── Application state ───────────────────────────────────────────────── */
static uint8_t  g_led_state      = 0;
static uint32_t g_uptime_sec     = 0;
static uint32_t g_events_total   = 0;
static uint32_t g_events_dropped = 0;



static void producer_timer(void)
{
    static uint32_t last_tick = 0;
    uint32_t now = timer_get_millis();

    if ((now - last_tick) >= 1000u) {
        last_tick = now;

        if (!eventq_push(&g_queue,
                         EVENT_TIMER_TICK,
                         PRIORITY_LOW,
                         now,
                         (void *)0)) {
            g_events_dropped++;
        }
    }
}

static void producer_button(void)
{
    static uint8_t  confirmed_state = 1;   /* 1 = released (pull-up) */
    static uint8_t  raw_prev        = 1;
    static uint32_t bounce_start    = 0;

    uint8_t  raw = gpio_read(PORT_D, BTN_PIN);
    uint32_t now = timer_get_millis();

    /* Did the raw level just change? Start the debounce timer */
    if (raw != raw_prev) {
        raw_prev     = raw;
        bounce_start = now;
    }

    /* Has the level been stable for DEBOUNCE_MS? */
    if ((now - bounce_start) >= DEBOUNCE_MS) {
        if (raw != confirmed_state) {
            confirmed_state = raw;

            /* Falling edge = GPIO_LOW = button physically pressed */
            if (confirmed_state == GPIO_LOW) {
                if (!eventq_push(&g_queue,
                                 EVENT_BUTTON_PRESSED,
                                 PRIORITY_NORMAL,
                                 now,
                                 (void *)0)) {
                    g_events_dropped++;
                }
            }
        }
    }
}

static void producer_uart(void)
{
    static char    cmd_buf[EVENTQ_CMD_LEN];
    static uint8_t cmd_idx = 0;

    while (uart_rx_available()) {
        char c = uart_read_byte();

        if (c == '\r' || c == '\n') {
            if (cmd_idx > 0) {
                cmd_buf[cmd_idx] = '\0';
                if (!eventq_push(&g_queue,
                                 EVENT_UART_CMD,
                                 PRIORITY_HIGH,
                                 timer_get_millis(),
                                 cmd_buf)) {
                    g_events_dropped++;
                }
                cmd_idx = 0;
            }
        } else if (cmd_idx < (EVENTQ_CMD_LEN - 1u)) {
            cmd_buf[cmd_idx++] = c;
        }
        /* else: buffer full, silently drop the character */
    }
}


static uint8_t str_eq(const char *a, const char *b)
{
    while (*a && *b) {
        if (*a != *b) return 0;
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0') ? 1u : 0u;
}

/* Helper: convert priority to a printable 4-char label */
static const char *priority_label(EventPriority p)
{
    switch (p) {
        case PRIORITY_HIGH:   return "HIGH";
        case PRIORITY_NORMAL: return "NORM";
        case PRIORITY_LOW:    return "LOW ";
        default:              return "??? ";
    }
}

static void handler_timer_tick(const Event *e)
{
    g_uptime_sec++;

    /* 1-second heartbeat toggle */
    g_led_state = !g_led_state;
    gpio_write(APP_LED_PORT, APP_LED_PIN,
               g_led_state ? GPIO_HIGH : GPIO_LOW);

    uart_print("[LOW ][TICK] t=");
    uart_print_num(e->timestamp);
    uart_print("ms  uptime=");
    uart_print_num(g_uptime_sec);
    uart_print("s  led=");
    uart_println(g_led_state ? "ON" : "OFF");
}

static void handler_button_pressed(const Event *e)
{

    g_led_state = !g_led_state;
    gpio_write(APP_LED_PORT, APP_LED_PIN,
               g_led_state ? GPIO_HIGH : GPIO_LOW);

    uart_print("[NORM][BTN ] BUTTON_PRESSED at t=");
    uart_print_num(e->timestamp);
    uart_print("ms  led=");
    uart_println(g_led_state ? "ON" : "OFF");
}

static void handler_uart_cmd(const Event *e)
{
    uart_print("[HIGH][CMD ] \"");
    uart_print(e->cmd);
    uart_println("\"");

    if (str_eq(e->cmd, "help")) {
        uart_println("  Commands:");
        uart_println("    help      - show this list");
        uart_println("    status    - print system state");
        uart_println("    led on    - turn LED on");
        uart_println("    led off   - turn LED off");
        uart_println("    queue     - show queue depth");
    }
    else if (str_eq(e->cmd, "status")) {
        uart_print("  uptime=");
        uart_print_num(g_uptime_sec);
        uart_print("s  led=");
        uart_print(g_led_state ? "ON" : "OFF");
        uart_print("  events=");
        uart_print_num(g_events_total);
        uart_print("  dropped=");
        uart_print_num(g_events_dropped);
        uart_println("");
    }
    else if (str_eq(e->cmd, "led on")) {
        g_led_state = 1;
        gpio_write(APP_LED_PORT, APP_LED_PIN, GPIO_HIGH);
        uart_println("  LED -> ON");
    }
    else if (str_eq(e->cmd, "led off")) {
        g_led_state = 0;
        gpio_write(APP_LED_PORT, APP_LED_PIN, GPIO_LOW);
        uart_println("  LED -> OFF");
    }
    else if (str_eq(e->cmd, "queue")) {
        uart_print("  depth=");
        uart_print_num(eventq_depth(&g_queue));
        uart_print("/");
        uart_print_num(EVENTQ_SIZE);
        uart_println("");
    }
    else {
        uart_print("  [ERR] unknown command: \"");
        uart_print(e->cmd);
        uart_println("\"  (type 'help')");
    }
}



static void dispatch_one(void)
{
    Event e;
    if (!eventq_pop(&g_queue, &e)) {
        return;   /* queue empty this iteration */
    }

    g_events_total++;

    uint32_t latency = timer_get_millis() - e.enqueue_time;
    (void)latency;   /* used in log lines below */

    switch (e.type) {
        case EVENT_TIMER_TICK:
            handler_timer_tick(&e);
            break;
        case EVENT_BUTTON_PRESSED:
            handler_button_pressed(&e);
            break;
        case EVENT_UART_CMD:
            handler_uart_cmd(&e);
            break;
        default:
            uart_println("[WARN] unknown event type ignored");
            break;
    }
}




int main(void)
{
  
    gpio_init(APP_LED_PORT, APP_LED_PIN, GPIO_MODE_OUTPUT);
    gpio_init(PORT_D, BTN_PIN, GPIO_MODE_INPUT_PU);
    uart_init(115200);
    timer_init();

    /* ── Framework initialisation ── */
    eventq_init(&g_queue);

    /* ── Start with LED off ── */
    gpio_write(APP_LED_PORT, APP_LED_PIN, GPIO_LOW);

    /* ── Startup banner ── */
    uart_println("================================");
    uart_println("  Event Queue Framework v1.0");
    uart_println("  Project-9  VSDSquadron Mini");
    uart_println("================================");
    uart_println("  Producers: TIMER | BTN | UART");

    while (1) {
        producer_timer();
        producer_button();
        producer_uart();
        dispatch_one();
    }
}