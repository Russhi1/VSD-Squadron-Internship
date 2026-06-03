/*
 * buzzer.h — Software PWM Buzzer Library
 * VSD Squadron Mini (CH32V003F4U6)
 *
 * Drives a passive buzzer on PC3 by toggling the pin at audio frequency
 * using millis-based timing. No hardware PWM peripheral needed.
 *
 * Usage pattern:
 *   buzzer_start(500)   — beep for 500ms
 *   buzzer_tick()       — call every main loop iteration (non-blocking)
 *   buzzer_is_done()    — returns 1 when pattern finished
 *   buzzer_stop()       — stop immediately (e.g., on disarm)
 *
 * Frequency: toggle every 1ms = 500Hz square wave (audible on all buzzers)
 */

#ifndef BUZZER_H
#define BUZZER_H

#include <stdint.h>

#define BUZZER_PORT   PORT_C
#define BUZZER_PIN    3

/*
 * buzzer_init — configure PC3 as output.
 * Call once during system initialisation.
 */
void buzzer_init(void);

/*
 * buzzer_start — begin a buzzer tone for duration_ms milliseconds.
 * If already active, resets the duration.
 */
void buzzer_start(uint32_t duration_ms);

/*
 * buzzer_stop — silence the buzzer immediately.
 */
void buzzer_stop(void);

/*
 * buzzer_tick — must be called every main loop iteration.
 * Handles pin toggling and duration tracking.
 * Non-blocking: returns immediately.
 */
void buzzer_tick(void);

/*
 * buzzer_is_done — returns 1 once after a pattern completes naturally.
 * Returns 0 at all other times.
 * Calling this function resets the done flag, so call it exactly once
 * per check — the main producer reads it to push EVENT_BUZZER_DONE.
 */
uint8_t buzzer_is_done(void);

/*
 * buzzer_is_active — returns 1 if currently beeping, 0 if silent.
 */
uint8_t buzzer_is_active(void);

#endif /* BUZZER_H */