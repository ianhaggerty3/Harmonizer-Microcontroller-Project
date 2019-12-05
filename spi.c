/*
 * spi.c
 *
 *  Created on: Nov 7, 2019
 *      Author: Ian Haggerty
 */

#include "stm32f0xx.h"

#include "spi.h"

void nano_wait(unsigned long n) {
    asm(    "        mov r0,%0\n"

            "repeat: sub r0,#83\n"

            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

int lookup_id(uint8_t * arr, int len, int id) {
    int i;

    for (i = 0; i < len; i++) {
        if (id == arr[i]) {
            return i;
        }
    }
    return -1;
}

uint8_t device_lookup(uint8_t base) {
	// Yet to be implemented; may want to return a pointer to GPIOA or something
	//Returns either 11, 12, 8, or 9
	uint8_t output;
	if(((base >> 2) & 0x3) == 0){
		output = 8;
	}
	else if (((base >> 2) & 0x3) == 1){
		output = 9;
	}
	else if (((base >> 2) & 0x3) == 2){
		output = 11;
	}
	else {
		output = 12;
	}
	return output;
}

void address_lookup(uint8_t * address_array, uint8_t base, uint16_t offset) {
	uint32_t address;

	address = (MEM_SIZE / MEM_SEGMENTS) * (base & 0x3);
	address += offset;

	address_array[0] = (address & 0xff0000) >> (2 * 8);
	address_array[1] = (address & 0x00ff00) >> (1 * 8);
	address_array[2] = (address & 0x0000ff) >> (0 * 8);
}

void init_dma(void) {
	RCC->AHBENR |= RCC_AHBENR_DMA1EN;


	// Channel 1 is for the ADC based on the FRM

	// Channel 4 is for SPI2 Rx
	DMA1_Channel4->CPAR = &SPI2->DR;
	DMA1_Channel4->CCR &= ~DMA_CCR_DIR;
	DMA1_Channel4->CCR |= DMA_CCR_MINC;

	DMA1_Channel4->CCR &= ~DMA_CCR_MSIZE;
	DMA1_Channel4->CCR &= ~DMA_CCR_PSIZE;

	DMA1_Channel4->CCR |= DMA_CCR_TCIE;

	// Channel 5 is for SPI2 Tx
	DMA1_Channel5->CPAR = &SPI2->DR;
	DMA1_Channel5->CCR |= DMA_CCR_DIR;
	DMA1_Channel5->CCR |= DMA_CCR_MINC;

	DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
	DMA1_Channel5->CCR &= ~DMA_CCR_PSIZE;

	DMA1_Channel5->CCR |= DMA_CCR_TCIE;

	NVIC->ISER[0] |= 1 << DMA1_Channel4_5_IRQn;
}

void init_spi(void) {
	RCC->AHBENR |= RCC_AHBENR_GPIOBEN;
	RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
	RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;

	// Manual SS for memory chips output
	GPIOB->MODER |= (1 << (2 * 8));
	GPIOB->MODER |= (1 << (2 * 9));
	GPIOB->MODER |= (1 << (2 * 11));

	// Set up pins for alternate function
	GPIOB->MODER |= (2 << (2 * 12)); // This one is not actually used for analog
	GPIOB->MODER |= (2 << (2 * 13));
	GPIOB->MODER |= (2 << (2 * 14));
	GPIOB->MODER |= (2 << (2 * 15));

	// Set as master
	SPI2->CR1 |= SPI_CR1_MSTR;

	// Set clock divisor for baud rate as 4 (by setting bit 0), the highest the 23LC1024 can handle
//	SPI2->CR1 |= SPI_CR1_BR_0;
	SPI2->CR1 |= SPI_CR1_BR_2;

	SPI2->CR2 |= SPI_CR2_FRXTH;

	// Stay idle at zero and trigger on the first clock edge
	SPI2->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);

	// Manage SS with software
	SPI2->CR1 |= SPI_CR1_SSM;
	SPI2->CR1 |= SPI_CR1_SSI;

	GPIOB->BSRR = 1 << 8;
	GPIOB->BSRR = 1 << 9;
	GPIOB->BSRR = 1 << 11;

	// Enable SPI communication
	SPI2->CR1 |= SPI_CR1_SPE;
}

// Initiates the process of writing to one channel's RAM memory. DMA settings are reset in a transfer complete interrupt
void write_array_dma(uint8_t * array, uint8_t id, DMA_Channel_TypeDef * dma_channel, SPI_TypeDef * spi) {
	uint8_t address[3];
	int pin;

	pin = device_lookup(recording_locations[id]);

	GPIOB->BRR |= 1 << pin;

	address_lookup(address, recording_locations[id], recording_offsets[id]);

	while (spi->SR & SPI_SR_FTLVL);
	*(uint8_t *)&spi->DR = 0x02;
	*(uint8_t *)&spi->DR = address[0];
	*(uint8_t *)&spi->DR = address[1];
	*(uint8_t *)&spi->DR = address[2];

	dma_channel->CMAR = array;
	dma_channel->CNDTR = BUF_LEN;
	dma_channel->CCR |= DMA_CCR_EN;
	spi->CR2 |= SPI_CR2_TXDMAEN;
}

// Initiates the process of reading every channel that has been recorded so far; interrupts on DMA completion automatically handle the rest.s
void read_array_dma(uint8_t * array, uint8_t id, DMA_Channel_TypeDef * dma_channel, SPI_TypeDef * spi) {
	uint16_t i;
	uint8_t current_element;
	uint8_t address[3];

	int pin;
	pin = device_lookup(recording_locations[id]);
	GPIOB->BRR |= 1 << pin;

	address_lookup(address, recording_locations[id], recording_offsets[id]);

	while (spi->SR & SPI_SR_FTLVL);
	*(uint8_t *)&spi->DR = 0x03;
	*(uint8_t *)&spi->DR = address[0];
	*(uint8_t *)&spi->DR = address[1];
	*(uint8_t *)&spi->DR = address[2];

	while (spi->SR & SPI_SR_FTLVL);
	while (spi->SR & SPI_SR_FRLVL) current_element = spi->DR;

	dma_channel->CMAR = array;
	dma_channel->CNDTR = BUF_LEN;
	dma_channel->CCR |= DMA_CCR_EN;
	spi->CR2 |= SPI_CR2_RXDMAEN;
	spi->CR1 |= SPI_CR1_RXONLY;
}

void DMA1_Channel4_5_IRQHandler(void) {
	uint8_t current_element;
	uint8_t current_id;
	int pin;

	while (SPI2->SR & SPI_SR_FTLVL);

	if (DMA1_Channel4->CCR & DMA_CCR_EN) {
		// Reception
		current_id = playback_ids[num_read];
		recording_offsets[current_id] += BUF_LEN;

		pin = device_lookup(recording_locations[current_id]);
		GPIOB->BSRR |= 1 << pin;

		num_read++;

		SPI2->CR1 &= ~SPI_CR1_RXONLY;
		SPI2->CR2 &= ~SPI_CR2_RXDMAEN;
		while (SPI2->SR & SPI_SR_FRLVL) current_element = SPI2->DR;
		DMA1->IFCR |= DMA_IFCR_CTCIF4;
		DMA1_Channel4->CCR &= ~DMA_CCR_EN;

		if (num_read >= num_to_read) {
			// This would be where we could make a combined values array if we wanted; we have all of the necessary parts for it
			queues_read++;
			return;
		}

		// need a current_buf variable
		current_id = playback_ids[num_read];
		read_array_dma(recordings_buf[current_id], current_id, DMA1_Channel4, SPI2);

	} else if (DMA1_Channel5->CCR & DMA_CCR_EN) {
		// Transmission
		current_id = recording_ids[num_recordings];
		pin = device_lookup(recording_locations[current_id]);
		GPIOB->BSRR |= 1 << pin;
		recording_offsets[current_id] += BUF_LEN;
		DMA1->IFCR |= DMA_IFCR_CTCIF5;
		SPI2->CR2 &= ~SPI_CR2_TXDMAEN;
		// Indicates we are done transmitting; do not need to do anything else, that will be handled by our keyboard functionality
		DMA1_Channel5->CCR &= ~DMA_CCR_EN;
	}
}

void initialize_locations(void) {
	int i;

	for (i = 0; i < NUM_CHANNELS; i++) {
		recording_locations[i] = i;
	}
}



