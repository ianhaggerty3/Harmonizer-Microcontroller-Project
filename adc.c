#include "stm32f0xx.h"
#include "stm32f0_discovery.h"

#include "adc.h"
#include "spi.h"

uint8_t currrec = 0;

void setup_gpio_adc() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
    GPIOA->MODER |= 0x33c;
}

//setup_timers(9, 1199);
void setup_timers(int psc, int arr){
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    TIM2->PSC = psc;
    TIM2->ARR = arr;
    TIM2->CCR3 = 5;
    TIM2->CCMR2 &= ~TIM_CCMR2_OC3M;
    TIM2->CCMR2 |= TIM_CCMR2_OC3M_1 | TIM_CCMR2_OC3M_2;
    TIM2->DIER |= TIM_DIER_CC3DE;
    TIM2->CR2 |= TIM_CR2_CCDS | TIM_CR2_MMS_1 | TIM_CR2_MMS_2;
    TIM2->CCER |= TIM_CCER_CC3E;
    TIM2->CR1 |= TIM_CR1_CEN;
}

void setup_adc() {
    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    RCC->CR2 |= RCC_CR2_HSI14ON;
    while(!(RCC->CR2 & RCC_CR2_HSI14RDY));
    ADC1->CFGR1 &= ~ADC_CFGR1_RES;
    ADC1->CFGR1 |= ADC_CFGR1_RES_1 | ADC_CFGR1_CONT;// | ADC_CFGR1_DMAEN;
    ADC1->CHSELR = 0;
    ADC1->CHSELR |= 1 << 1; //Select Channel 1
    ADC1->CR |= ADC_CR_ADEN;
    while(!(ADC1->ISR & ADC_ISR_ADRDY));
    ADC1->CR |= ADC_CR_ADSTART;
    while(!(ADC1->CR & ADC_CR_ADSTART));
}

void setup_dac() {
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR |= DAC_CR_EN1 | DAC_CR_DMAEN1;
}

void dma_playback() {
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE | DMA_CCR_PINC);
    DMA1_Channel1->CCR |= (DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_DIR | DMA_CCR_MINC);
//	DMA1_Channel1->CCR |= (DMA_CCR_DIR | DMA_CCR_MINC);
    DMA1_Channel1->CNDTR = BUF_LEN;
    DMA1_Channel1->CMAR = (uint32_t) output;
    DMA1_Channel1->CPAR = (uint32_t) (&(DAC->DHR12R1));
//    DMA1_Channel1->CPAR = (uint32_t) (&(DAC->DHR8R1));
    DMA1_Channel1->CCR |= DMA_CCR_EN | DMA_CCR_TCIE;
    NVIC->ISER[0] |= 1<<DMA1_Channel1_IRQn;
}

void dmarecord(int chan) {
	currrec = chan;
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    DMA1_Channel1->CCR &= ~(DMA_CCR_MSIZE | DMA_CCR_PSIZE | DMA_CCR_PINC | DMA_CCR_DIR);
//    DMA1_Channel1->CCR |= (DMA_CCR_MSIZE_0 | DMA_CCR_PSIZE_0 | DMA_CCR_MINC);
    DMA1_Channel1->CCR |= DMA_CCR_MINC;
    DMA1_Channel1->CNDTR = BUF_LEN;
    DMA1_Channel1->CMAR = (uint32_t) recordings_buf[chan];
    DMA1_Channel1->CPAR = (uint32_t) (&(ADC1->DR));
    DMA1_Channel1->CCR |= DMA_CCR_EN | DMA_CCR_TCIE;
    NVIC->ISER[0] |= 1<<DMA1_Channel1_IRQn;
}

void generate_output(void) {
	int i;
	int j;
	int current_id;

	for (i = 1; i < BUF_LEN; i++) {
		// Temporary fix to combining audio channels by just using channel 0.
		output[i] = 0;
		for (j = 0; j < num_to_read; j++) {
			current_id = playback_ids[j];
			output[i] += (recordings_buf[current_id][i] << 4) / num_to_read;
//			output[i] += recordings_buf[current_id][i];
		}
	}
	output[0] = output[1];
}

int get_continuous(int ch) {
	return (recording_locations[ch] >> 4) & 0x1;
}

void update_queue(void) {
	int i;
	int current_id;

	for (i = 0; i < num_to_read; i++) {
		current_id = playback_ids[i];
		if (recording_offsets[current_id] >= CHANNEL_BYTES) {
			recording_offsets[current_id] = 0;
			if (get_continuous(current_id)) {
				continue;
			}
			playback_ids[i] = playback_ids[num_to_read - 1];
			num_to_read--;
			i--;
		}
	}
}

void DMA1_Channel1_IRQHandler() {
	// called on transfer complete;
    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
    int current_id;

	DMA1->IFCR |= DMA_IFCR_CTCIF1;

    if (DMA1_Channel1->CCR & DMA_CCR_DIR) {
    	// Playing back audio

    	DMA1_Channel1->CNDTR = BUF_LEN;
        DMA1_Channel1->CMAR = (uint32_t) output;
    	DMA1_Channel1->CCR |= DMA_CCR_EN;

    	// Generate output afterwards to avoid taking too long before writing to the DAC
    	generate_output();
    	update_queue();

    	num_read = 0;

    	if (num_to_read == 0) {
    	    DMA1_Channel1->CCR &= ~DMA_CCR_EN;
        	DMA1_Channel1->CNDTR = BUF_LEN;
            return;
    	}

    	current_id = playback_ids[0];
    	read_array_dma(recordings_buf[current_id], current_id, DMA1_Channel4, SPI2);

    } else {
    	// Recording audio
        DMA1_Channel1->CMAR = (uint32_t) recordings_buf[currrec];
    	write_array_dma(recordings_buf[currrec], currrec, DMA1_Channel5, SPI2);
		if (recording_offsets[currrec] >= CHANNEL_BYTES) {
			// Done recording
		    GPIOA->ODR &= ~(1<<8);
			recording_offsets[currrec] = 0;
			recording_ids[num_recordings] = currrec;
			num_recordings++;
			return;
		}
		DMA1_Channel1->CNDTR = BUF_LEN;
		DMA1_Channel1->CCR |= DMA_CCR_EN;
    }
}
