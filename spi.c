/*
 * spi.c
 *
 *  Created on: Nov 7, 2019
 *      Author: Ian Haggerty
 */

#include "stm32f0xx.h"

#include "spi.h"

int lookup_id(uint8_t * arr, int len, int id) {
    int i;

    for (i = 0; i < len; i++) {
        if (id == arr[i]) {
            return 1;
        }
    }
    return -1;
}

uint8_t device_lookup(uint8_t base) {
	// Yet to be implemented; may want to return a pointer to GPIOA or something

	return 0x00;
}

void address_lookup(uint8_t * address_array, uint8_t base, uint16_t offset) {
	uint32_t address;
	if (base > (MEM_SEGMENTS - 1)) {
		return;
	}
	address = (MEM_SIZE / MEM_SEGMENTS) * base;
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
	GPIOB->MODER |= (1 << (2 * 11));

	// Set up pins for alternate function
	GPIOB->MODER |= (2 << (2 * 12));
	GPIOB->MODER |= (2 << (2 * 13));
	GPIOB->MODER |= (2 << (2 * 14));
	GPIOB->MODER |= (2 << (2 * 15));

	// Set as master
	SPI2->CR1 |= SPI_CR1_MSTR;

	// Set clock divisor for baud rate as 4 (by setting bit 0), the highest the 23LC1024 can handle
	SPI2->CR1 |= SPI_CR1_BR_0;
//	SPI2->CR1 |= SPI_CR1_BR_2;

	SPI2->CR2 |= SPI_CR2_FRXTH;

	// Stay idle at zero and trigger on the first clock edge
	SPI2->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);

	// Manage SS with software
	SPI2->CR1 |= SPI_CR1_SSM;
	SPI2->CR1 |= SPI_CR1_SSI;

	GPIOB->BSRR = 1 << 11;

	// Enable SPI communication
	SPI2->CR1 |= SPI_CR1_SPE;
}

void write_array(uint8_t * array, uint16_t len, uint8_t address) {
	// We need 24 bits for all of the addresses
	uint16_t i;
	uint8_t current_element;
	uint8_t address_array[3];

	address_lookup(address_array, address, 0);

	GPIOB->BRR |= 1 << 11;

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x02;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address_array[0];
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address_array[1];
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address_array[2];

	for (i = 0; i < len; i++) {
		current_element = array[i];
		while (!(SPI2->SR & SPI_SR_TXE));
		*(uint8_t *)&SPI2->DR = current_element;
	}

	while (SPI2->SR & SPI_SR_BSY);

	GPIOB->BSRR |= 1 << 11;
}

void read_array(uint8_t * array, uint16_t len, uint8_t address) {
	uint16_t i;
	uint8_t current_element;
	uint8_t address_array[3];

	address_lookup(address_array, address, 0);

	GPIOB->BRR |= 1 << 11;

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x03;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address_array[0];
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address_array[1];
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address_array[2];

	while (SPI2->SR & SPI_SR_BSY);

	while (SPI2->SR & SPI_SR_FRLVL) current_element = SPI2->DR;

	SPI2->CR1 |= SPI_CR1_RXONLY;
	for (i = 0; i < len; i++) {
		while (!(SPI2->SR & SPI_SR_RXNE));
		current_element = (uint8_t) SPI2->DR;
		array[i] = current_element;
	}
	SPI2->CR1 &= ~SPI_CR1_RXONLY;

	GPIOB->BSRR |= 1 << 11;
}

// Initiates the process of writing to one channel's RAM memory. DMA settings are reset in a transfer complete interrupt
void write_array_dma(uint8_t * array, uint8_t id, DMA_Channel_TypeDef * dma_channel, SPI_TypeDef * spi) {
	uint8_t address[3];

	GPIOB->BRR |= 1 << 11;

	address_lookup(address, recording_location_and_base_addrs[id], recording_offsets[id]);

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

	GPIOB->BRR |= 1 << 11;

	address_lookup(address, recording_location_and_base_addrs[id], recording_offsets[id]);

	while (spi->SR & SPI_SR_FTLVL);
	*(uint8_t *)&spi->DR = 0x03;
	*(uint8_t *)&spi->DR = address[0];
	*(uint8_t *)&spi->DR = address[1];
	*(uint8_t *)&spi->DR = address[2];

	while (spi->SR & SPI_SR_BSY);
	while (spi->SR & SPI_SR_FRLVL) current_element = spi->DR;

	dma_channel->CMAR = array;
	dma_channel->CNDTR = BUF_LEN;
	dma_channel->CCR |= DMA_CCR_EN;
	spi->CR2 |= SPI_CR2_RXDMAEN;
	spi->CR1 |= SPI_CR1_RXONLY;
}

void DMA1_Channel4_5_IRQHandler(void) {
	uint8_t current_element;

	while (SPI2->SR & SPI_SR_FTLVL);
	GPIOB->BSRR |= 1 << 11;
	if (DMA1_Channel4->CCR & DMA_CCR_EN) {
		// Reception
		num_read++;

		SPI2->CR1 &= ~SPI_CR1_RXONLY;
		SPI2->CR2 &= ~SPI_CR2_RXDMAEN;
		while (SPI2->SR & SPI_SR_FRLVL) current_element = SPI2->DR;

		DMA1->IFCR |= DMA_IFCR_CTCIF4;
		DMA1_Channel4->CCR &= ~DMA_CCR_EN;

		if (num_read >= num_to_read) {
			// This would be where we could make a combined values array if we wanted; we have all of the necessary parts for it
			return;
		}

		// need a current_buf variable
		uint8_t current_id = playback_ids[num_read];
		read_array_dma(recordings_buf2[current_id], current_id, DMA1_Channel4, SPI2);

	} else if (DMA1_Channel5->CCR & DMA_CCR_EN) {
		// Transmission
		DMA1->IFCR |= DMA_IFCR_CTCIF5;
		SPI2->CR2 &= ~SPI_CR2_TXDMAEN;
		// Indicates we are done transmitting; do not need to do anything else, that will be handled by our keyboard functionality
		DMA1_Channel5->CCR &= ~DMA_CCR_EN;

		// assume it is a new recording; eventually we will have to check
		num_recordings++;
	}
}



















