#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char totalbuff[12][128];
char buttons[16] = {0};
char output[128];
short int chanloops[16];
int soundloops = 0;
char record = 0;
char currrec = 0;
int offset = 0;

void setup_gpio_adc(void);
void setup_timers(int psc, int arr);
void setup_adc(void);
void setup_dac(void);
void setup_dma(uint32_t location);
void TIM3_IRQHandler(void);
void dmaplayback(void);
void dmarecord(int chan);
void DMA1_Channel1_IRQHandler(void);
