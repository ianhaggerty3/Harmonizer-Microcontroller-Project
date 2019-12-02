/**
  ******************************************************************************
  * @file    main.c
  * @author  Ac6
  * @version V1.0
  * @date    01-December-2013
  * @brief   Default main function.
  ******************************************************************************
*/


#include "stm32f0xx.h"
#include "stm32f0_discovery.h"

#include "adc.h"
#include "spi.h"

#define UNIT_TEST

#ifdef UNIT_TEST

void test_array_operation(void) {
	uint8_t array[] = {255, 1, 2, 3, 4, 5};
	uint8_t check_array[6];
	int i;

	write_array(array, 6, 0x01);

	nano_wait(100000);

	read_array(check_array, 6, 0x01);

	for (i = 0; i < 6; i++) {
		if (array[i] != check_array[i]) {
			// Set a breakpoint here to check if the arrays are the same
			nano_wait(100);
		}
	}
	nano_wait(100);
}

void test_dma_array_operation(void) {
	int i;

	recording_ids[0] = 0;
	recording_ids[1] = 4;
	recording_ids[2] = 3;
	for (i = 0; i < BUF_LEN; i++) {
		recordings_buf1[0][i] = i;
		recordings_buf1[4][i] = i;
		recordings_buf1[3][i] = i;
	}
	recording_location_and_base_addrs[0] = 0x00;
	recording_location_and_base_addrs[4] = 0x01;
	recording_location_and_base_addrs[3] = 0x02;
	for (i = 0; i < NUM_CHANNELS; i++) {
		recording_offsets[i] = 0;
	}

	num_recordings = 0;
	write_array_dma(recordings_buf1[0], 0, DMA1_Channel5, SPI2);
	while (num_recordings == 0);
	write_array_dma(recordings_buf1[4], 4, DMA1_Channel5, SPI2);
	while (num_recordings == 1);
	write_array_dma(recordings_buf1[3], 3, DMA1_Channel5, SPI2);
	while (num_recordings == 2);

	read_array_dma(recordings_buf2[0], 0, DMA1_Channel4, SPI2);
	while (num_read != 3);

	for (i = 0; i < BUF_LEN; i++) {
		if (recordings_buf2[0][i] != i
		 || recordings_buf2[4][i] != i
		 || recordings_buf2[3][i] != i) {
			nano_wait(1);
		}
	}
	nano_wait(1);
}

void test_address_lookup(void) {
	int i;
	uint8_t input_address[3];
	uint8_t expected_address[3] = {0x01, 0x02, 0x00};

	address_lookup(input_address, 0x200, 2);
	for (i = 0; i < 3; i++) {
		if (input_address[i] != expected_address[i]) {
			nano_wait(100);
		}
	}
	nano_wait(100);
}

void test_buf_count(void) {
	if (BUF_COUNT != 0x100) {
		nano_wait(100);
	}
	nano_wait(100);
}

#endif

int main(void) {

	// Note: we have 8 kB of memory on the chip

	init_spi();
	init_dma();

	// Matt's main
	setup_timers(9, 1199);
//	setup_dma();
	// end of Matt's main

#ifdef UNIT_TEST
//	test_array_operation();
	test_dma_array_operation();
	test_address_lookup();
	test_buf_count();
#endif

//	DMA1

	for(;;);
}
