/*
 * spi.c
 *
 *  Created on: Nov 7, 2019
 *      Author: Ian Haggerty
 */

#include "stm32f0xx.h"

#include "spi.h"

uint8_t device_lookup(uint8_t base) {
	// Yet to be implemented; may want to return a pointer to GPIOA or something

	return 0x00;
}

void address_lookup(uint8_t * address_array, uint16_t offset, uint8_t base) {
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

	// Channel 2 is for SPI1 Rx
	DMA1_Channel2->CPAR = &SPI1->DR;
	DMA1_Channel2->CCR &= ~DMA_CCR_DIR;
	DMA1_Channel2->CCR |= DMA_CCR_MINC;
	// Set memory address and count later on

	DMA1_Channel2->CCR &= ~DMA_CCR_MSIZE;
	DMA1_Channel2->CCR &= ~DMA_CCR_PSIZE;

	DMA1_Channel2->CCR |= DMA_CCR_EN;

	// Channel 3 is for SPI1 Tx
	DMA1_Channel3->CPAR = &SPI1->DR;
	DMA1_Channel3->CCR |= DMA_CCR_DIR;
	DMA1_Channel3->CCR |= DMA_CCR_MINC;

	DMA1_Channel3->CCR &= ~DMA_CCR_MSIZE;
	DMA1_Channel3->CCR &= ~DMA_CCR_PSIZE;

	DMA1_Channel3->CCR |= DMA_CCR_EN;

	// Channel 4 is for SPI2 Rx
	DMA1_Channel4->CPAR = &SPI2->DR;
	DMA1_Channel4->CCR &= ~DMA_CCR_DIR;
	DMA1_Channel4->CCR |= DMA_CCR_MINC;

	DMA1_Channel4->CCR &= ~DMA_CCR_MSIZE;
	DMA1_Channel4->CCR &= ~DMA_CCR_PSIZE;

	DMA1_Channel4->CCR |= DMA_CCR_EN;

	// Channel 5 is for SPI2 Tx
	DMA1_Channel5->CPAR = &SPI2->DR;
	DMA1_Channel5->CCR |= DMA_CCR_DIR;
	DMA1_Channel5->CCR |= DMA_CCR_MINC;

	DMA1_Channel5->CCR &= ~DMA_CCR_MSIZE;
	DMA1_Channel5->CCR &= ~DMA_CCR_PSIZE;

	DMA1_Channel5->CCR |= DMA_CCR_EN;
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

	// Set clock divisor for baud rate as 4, the highest the 23LC1024 can handle
	SPI2->CR1 |= SPI_CR1_BR_0;

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

	GPIOB->BRR |= 1 << 11;

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x02;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address;

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

	GPIOB->BRR |= 1 << 11;

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x03;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address;

	while (SPI2->SR & SPI_SR_BSY);

	while (SPI2->SR & SPI_SR_FRLVL) current_element = SPI2->DR;


	SPI2->CR1 |= SPI_CR1_RXONLY;
	for (i = 0; i < len; i++) {
		while (!(SPI2->SR & SPI_SR_RXNE));
		current_element = SPI2->DR;
		array[i] = current_element;
	}
	SPI2->CR1 &= ~SPI_CR1_RXONLY;

	GPIOB->BSRR |= 1 << 11;
}
