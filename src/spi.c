/*
 * spi.c
 *
 *  Created on: Nov 7, 2019
 *      Author: Ian Haggerty
 */

#include "stm32f0xx.h"

#include "spi.h"

void init_spi() {
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

	// Configure for bidirectional mode and set as master
	//SPI2->CR1 |= (SPI_CR1_BIDIMODE | SPI_CR1_BIDIOE | SPI_CR1_MSTR);

	SPI2->CR1 |= SPI_CR1_MSTR;

	// Set clock divisor for baud rate as 4, the highest this  device can handle
	//SPI2->CR1 |= SPI_CR1_BR_0;

	// Lower Baud rate for testing
	SPI2->CR1 |= (SPI_CR1_BR_0 | SPI_CR1_BR_2);

	// Set 8-bit word size
	//SPI2->CR2 |= (SPI_CR2_DS_0 | SPI_CR2_DS_1 | SPI_CR2_DS_2);

	SPI2->CR2 |= SPI_CR2_FRXTH;

	// Stay idle at zero and trigger on the first clock edge
	SPI2->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);

	// Manage SS with software
	SPI2->CR1 |= SPI_CR1_SSM;
	SPI2->CR1 |= SPI_CR1_SSI;


	GPIOB->BSRR = 1 << 11;

	// SPI2->CR1 |= (SPI_CR1_SSM | SPI_CR1_SSI);

	// NSS pulse management and slave select output
	// SPI2->CR2 |= (SPI_CR2_NSSP | 0);

	// Enable SPI communication
	SPI2->CR1 |= SPI_CR1_SPE;
}

void set_mode() {
	uint8_t mode_register;

	GPIOB->BRR |= 1 << 11;

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x05;
	while (!(SPI2->SR & SPI_SR_RXNE));
	mode_register = (uint8_t)SPI2->DR;

	// set mode to byte mode
	mode_register &= ~(0xc0);

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x01;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = mode_register;

	GPIOB->BSRR |= 1 << 11;
}

void write_byte(uint8_t byte, uint8_t address) {
	// We need 24 bits for all of the addresses

	GPIOB->BRR |= 1 << 11;

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x02;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = byte;

	GPIOB->BSRR |= 1 << 11;
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

uint8_t read_byte(uint8_t address) {
	uint16_t ret_byte;

	GPIOB->BRR |= 1 << 11;

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x03;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00; // This is to clock SCLK and doesn't mean anything as a write operation
	while (!(SPI2->SR & SPI_SR_RXNE));
	ret_byte = SPI2->DR;

	GPIOB->BSRR |= 1 << 11;

	uint8_t temp = (0xff00 & ret_byte) >> 8;

	return temp;
}

// Note: pass in the address of the subsection of the array that you want to write to
void read_array(uint8_t * array, uint16_t len, uint8_t address) {
	uint16_t i;
	uint8_t current_element;

	GPIOB->BRR |= 1 << 11;

	nano_wait(100);

	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x03;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = 0x00;
	while (!(SPI2->SR & SPI_SR_TXE));
	*(uint8_t *)&SPI2->DR = address;

	while (SPI2->SR & SPI_SR_BSY);

	for (i = 0; i < len; i++) {
		while (!(SPI2->SR & SPI_SR_RXNE));
		current_element = SPI2->DR;
		array[i] = current_element;
		while (SPI2->SR & SPI_SR_BSY);
	}

	while (SPI2->SR & SPI_SR_BSY);

	GPIOB->BSRR |= 1 << 11;
}
