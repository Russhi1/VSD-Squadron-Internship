/*
 * buzzer.c — Software PWM Buzzer Implementation
 *
 * The buzzer pin is toggled every BUZZER_TOGGLE_MS milliseconds.
 * At 1ms per toggle: full cycle = 2ms → 500Hz square wave.
 *
 * buzzer_tick() is called from the main loop on every iteration.
 * It uses timer_get_millis() for all timing — no blocking waits.
 *
 * When duration expires naturally, _done flag is set for one read.
 * The producer in main.c checks buzzer_is_done() and pushes
 * EVENT_BUZZER_DONE into the queue — demonstrating that even hardware
 * completion events flow through the event queue architecture.
 */

#include "buzzer.h"
#include "gpio.h"
#include "timer.h"

#define BUZZER_TOGGLE_MS   1u    /* toggle every 1ms = 500Hz */

static uint8_t  _active      = 0;
static uint8_t  _done        = 0;
static uint32_t _end_time    = 0;
static uint32_t _last_toggle = 0;

void buzzer_init(void)
{
    gpio_init(BUZZER_PORT, BUZZER_PIN, GPIO_MODE_OUTPUT);
    gpio_write(BUZZER_PORT, BUZZER_PIN, GPIO_LOW);
}

void buzzer_start(uint32_t duration_ms)
{
    uint32_t now = timer_get_millis();
    _active      = 1;
    _done        = 0;
    _end_time    = now + duration_ms;
    _last_toggle = now;
}

void buzzer_stop(void)
{
    _active = 0;
    _done   = 0;
    gpio_write(BUZZER_PORT, BUZZER_PIN, GPIO_LOW);
}

void buzzer_tick(void)
{
    if (!_active) return;

    uint32_t now = timer_get_millis();

    /*
     * Duration expired — silence and set done flag.
     * Main loop producer will read _done and push EVENT_BUZZER_DONE.
     */
    if (now >= _end_time) {
        _active = 0;
        _done   = 1;
        gpio_write(BUZZER_PORT, BUZZER_PIN, GPIO_LOW);
        return;
    }

    /* Toggle pin at BUZZER_TOGGLE_MS interval to generate tone */
    if ((now - _last_toggle) >= BUZZER_TOGGLE_MS) {
        _last_toggle = now;
        gpio_toggle(BUZZER_PORT, BUZZER_PIN);
    }
}

uint8_t buzzer_is_done(void)
{
    if (_done) {
        _done = 0;   /* auto-clear after one read */
        return 1;
    }
    return 0;
}

uint8_t buzzer_is_active(void)
{
    return _active;
}