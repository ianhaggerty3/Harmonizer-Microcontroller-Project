#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char totalbuff[16][100];
char buttons[16] = {0};
short int chanloops[16];
int soundloops = 0;
char record = 0;
char currrec = 0;
int offset = 0;

setup_gpio(){
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN | RCC_AHBENR_GPIOCEN;
    GPIOA->MODER |= 0x33c;

    GPIOC->MODER &= ~(3 << (0 * 2));
	GPIOC->MODER &= ~(3 << (1 * 2));
	GPIOC->MODER &= ~(3 << (2 * 2));
	GPIOC->MODER &= ~(3 << (3 * 2));
	GPIOC->MODER |= (1 << (0 * 2));
	GPIOC->MODER |= (1 << (1 * 2));
	GPIOC->MODER |= (1 << (2 * 2));
	GPIOC->MODER |= (1 << (3 * 2));

	// configure pull down resistors
	GPIOC->PUPDR &= ~(3 << (4 * 2));
	GPIOC->PUPDR &= ~(3 << (5 * 2));
	GPIOC->PUPDR &= ~(3 << (6 * 2));
	GPIOC->PUPDR &= ~(3 << (7 * 2));
	GPIOC->PUPDR |= (2 << (4 * 2));
	GPIOC->PUPDR |= (2 << (5 * 2));
	GPIOC->PUPDR |= (2 << (6 * 2));
	GPIOC->PUPDR |= (2 << (7 * 2));
}

void setup_timers(int psc, int arr){
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN | RCC_APB1ENR_TIM3EN;
    TIM6->PSC = psc;
    TIM6->ARR = arr;
    TIM6->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] = 1 <<TIM6_DAC_IRQn;

    TIM3->ARR = arr;
    TIM3->PSC = psc;
    TIM3->DIER |= TIM_DIER_UIE;
    NVIC->ISER[0] = 1 <<TIM3_IRQn;
}

void setup_adc() {
    /* Student code goes here */
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while(!(RCC->CR2 & RCC_CR2_HSI14RDY));
    ADC1->CFGR1 &= ~ADC_CFGR1_RES;
    ADC1->CFGR1 |= ADC_CFGR1_RES_1 | ADC_CFGR1_DMAEN;
    ADC1->CR |= ADC_CR_ADEN;
    while(!(ADC1->ISR & ADC_ISR_ADRDY));
    while((ADC1->CR & ADC_CR_ADSTART));
}

void setup_dma(uint32_t location){
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CCR &= !DMA_CCR_EN;
    DMA1_Channel1->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE| DMA_CCR_DIR);
    DMA1_Channel1->CCR |= DMA_CCR_MINC | DMA_CCR_TCIE;
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
    DMA1_Channel1->CMAR = (uint32_t)(location);
    DMA1_Channel1->CNDTR = 128;
}

void start_dma(){
    DMA1_Channel1->CCR |= DMA_CCR_EN;
}

void stop_dma(){
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
}

void TIM6_DAC_IRQHandler(){

}

void TIM3_IRQHandler(){
    totalbuff[currrec][offset] = read_adc_channel(1);
    offset++;
    if(offset >= 100){
        //Enable DMA
        offset = 0;
        soundloops++;
    }
    if(record == 0){
        TIM3->CR1 &= ~TIM_CR1_CEN;
        chanloops[currrec] = soundloops;
        soundloops = 0;
    }
}

int read_adc_channel(unsigned int channel) {
    /* Student code goes here */
    int hold = 0;
    ADC1->CHSELR = 0;
    ADC1->CHSELR |= (1<<channel);
    while(!(ADC1->ISR & ADC_ISR_ADRDY));
    ADC1->CR |= ADC_CR_ADSTART;
    while(!(ADC1->ISR & ADC_ISR_EOC));
    hold = ADC1->DR;
    return hold;
}

void record_audio(){
    TIM3->CR1 |= TIM_CR1_CEN;
    currrec = get_button();
    start_dma();
    while(record == 1);
    TIM3->CR1 &= ~TIM_CR1_CEN;
    stop_dma();
}
