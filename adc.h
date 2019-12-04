#ifndef ADC_H
#define ADC_H

void setup_gpio_adc(void);
void setup_timers(int psc, int arr);
void setup_adc(void);
void setup_dac(void);
void dmaplayback(void);
void dmarecord(int chan);
void generate_audio(void);
void DMA1_Channel1_IRQHandler(void);

#endif
