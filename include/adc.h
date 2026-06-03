/*
 * adc.h — ADC Single-Channel Read Library
 * VSD Squadron Mini (CH32V003F4U6)
 *
 * Supports 10-bit single software-triggered conversions.
 * Channel 2 (AIN2) = PC4 — used for LDR voltage divider.
 *
 * Returns values 0–1023:
 *   0    = 0V   (LDR fully covered, maximum resistance, dark)
 *   1023 = 3.3V (LDR in bright light, minimum resistance)
 */

#ifndef ADC_H
#define ADC_H

#include <stdint.h>

#define ADC_LDR_CHANNEL    2     /* PC4 = AIN2 */
#define ADC_MAX_VALUE      1023  /* 10-bit resolution */

/*
 * adc_init — configure ADC1 for single software-triggered conversions.
 * Must be called once before adc_read().
 */
void adc_init(void);

/*
 * adc_read — perform one blocking conversion on the given channel.
 * Returns 0–1023. Takes approximately 100us at 3MHz ADCCLK.
 */
uint16_t adc_read(uint8_t channel);

#endif /* ADC_H */