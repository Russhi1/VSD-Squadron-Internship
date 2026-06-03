/*
 * adc.c — ADC Implementation
 */

#include "adc.h"
#include <ch32v00x.h>

void adc_init(void)
{
    RCC->APB2PCENR |= RCC_APB2Periph_ADC1 | RCC_APB2Periph_GPIOC;

    GPIOC->CFGLR &= ~(0xFU << 16);

    RCC->CFGR0 &= ~(0x1FU << 11);
    RCC->CFGR0 |=  (0x18U << 11);

    ADC1->CTLR2 |= ADC_ADON;

    for (volatile uint32_t i = 0; i < 240; i++);

    ADC1->CTLR2 |= ADC_RSTCAL;
    while (ADC1->CTLR2 & ADC_RSTCAL);

    ADC1->CTLR2 |= ADC_CAL;
    while (ADC1->CTLR2 & ADC_CAL);
}

uint16_t adc_read(uint8_t channel)
{
    ADC1->RSQR3 = (uint32_t)(channel & 0x1FU);

    uint32_t shift = (uint32_t)channel * 3U;
    ADC1->SAMPTR2 &= ~(0x7U << shift);
    ADC1->SAMPTR2 |=  (0x7U << shift);

    ADC1->CTLR2 |= ADC_ADON;
    while (!(ADC1->STATR & ADC_FLAG_EOC));

    return (uint16_t)(ADC1->RDATAR & 0x03FFU);
}