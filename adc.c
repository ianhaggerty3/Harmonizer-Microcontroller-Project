#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "adc.h"
#include "spi.h"

short int chanloops[12];
int soundloops = 0;
char adc_record = 0;
char currrec = 0;
int offset = 0;
int record_offset;

void setup_gpio_adc() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN;
    GPIOA->MODER |= 0x33c;
}

//setup_timers(9, 1199);
void setup_timers(int psc, int arr){
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;// | RCC_APB1ENR_TIM3EN;
    TIM2->PSC = psc;
    TIM2->ARR = arr;
    TIM2->CCR3 = 5;
    TIM2->CCMR2 &= ~TIM_CCMR2_OC3M;
    TIM2->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;
    TIM2->DIER |= TIM_DIER_CC3DE;
    TIM2->CR2 |= TIM_CR2_CCDS | TIM_CR2_MMS_1 | TIM_CR2_MMS_2;
    TIM2->CCER |= TIM_CCER_CC3E;
    TIM2->CR1 |= TIM_CR1_CEN;
    //TIM2->DIER |= TIM_DIER_UIE;
    //NVIC->ISER[0] = 1 <<TIM2_IRQn;

    TIM3->ARR = arr;
    TIM3->PSC = psc;
    TIM3->DIER |= TIM_DIER_UIE;
    TIM3->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] = 1 <<TIM3_IRQn;
}

void setup_adc() {
    /* Student code goes here */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while(!(RCC->CR2 & RCC_CR2_HSI14RDY));
    ADC1->CFGR1 &= ~ADC_CFGR1_RES;
    ADC1->CFGR1 |= ADC_CFGR1_RES_1;// | ADC_CFGR1_DMAEN;
    ADC1->CR |= ADC_CR_ADEN;
    while(!(ADC1->ISR & ADC_ISR_ADRDY));
    while((ADC1->CR & ADC_CR_ADSTART));
}

void setup_dac() {
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR |= DAC_CR_EN1 | DAC_CR_DMAEN1;
}

void setup_dma(uint32_t location) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CCR &= !DMA_CCR_EN;
    DMA1_Channel1->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE| DMA_CCR_DIR);
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_TCIE;
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
    DMA1_Channel1->CMAR = (uint32_t)(location);
    DMA1_Channel1->CNDTR = 128;
}

// TODO: Possibly obsolete?
void TIM3_IRQHandler() {
//    recordings_buf[currrec][offset] = read_adc_channel(1);
    offset++;
    if(offset >= 100){
        //Enable DMA
        offset = 0;
        soundloops++;
    }
    if(adc_record == 0){
        TIM3->CR1 &= ~TIM_CR1_CEN;
        chanloops[currrec] = soundloops;
        soundloops = 0;
    }
}

void dmaplayback() {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE | DMA_CCR_PINC);
//    DMA1_Channel1->CCR |= (DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_DIR | DMA_CCR_MINC);
	DMA1_Channel1->CCR |= (DMA_CCR_DIR | DMA_CCR_MINC);
    DMA1_Channel1->CNDTR = BUF_LEN;
    DMA1_Channel1->CMAR = (uint32_t) output;
    DMA1_Channel1->CPAR = (uint32_t) (&(DAC->DHR8R1));
    DMA1_Channel1->CCR |= DMA_CCR_EN | DMA_CCR_TCIE;
    NVIC->ISER[0] |= 1<<DMA1_Channel1_IRQn;
}

void dmarecord(int chan) {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE | DMA_CCR_PINC | DMA_CCR_DIR);
//    DMA1_Channel1->CCR |= (DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC);
    DMA1_Channel1->CCR |= DMA_CCR_MINC;
    DMA1_Channel1->CNDTR = BUF_LEN;
    DMA1_Channel1->CMAR = (uint32_t) &(recordings_buf[chan]);
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
    DMA1_Channel1->CCR |= DMA_CCR_EN | DMA_CCR_TCIE;
    NVIC->ISER[0] |= 1<<DMA1_Channel1_IRQn;
}

void generate_output(void) {
	int i;

	for (i = 0; i < BUF_LEN; i++) {
		// Temporary fix to combining audio channels by just using channel 0.
		output[i] = recordings_buf[0][i];
	}
}

void DMA1_Channel1_IRQHandler() {
	// called on transfer complete;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    int current_id;

    if (DMA1_Channel1->CCR & DMA_CCR_DIR) {
    	// Playing back audio
    	DMA1_Channel1->CNDTR = BUF_LEN;
    	DMA1_Channel1->CCR |= DMA_CCR_EN;

    	// Generate output afterwards to avoid taking too long before writing to the DAC
    	generate_output();

    	current_id = playback_ids[0];
//    	if ()
    	read_array_dma(recordings_buf[current_id], current_id, DMA1_Channel4, SPI2);


    } else {
    	// Recording audio


    }
}

