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
#include "keypad.h"
#include "spi.h"

//#define UNIT_TEST

#ifdef UNIT_TEST

void zero_arr(uint8_t * arr) {
	int i;

	for (i = 0; i < BUF_LEN; i++) {
		arr[i] = 0;
	}
}

void test_dma_array_operation(void) {
	int i;

	recording_ids[0] = 0;
	recording_ids[1] = 4;
	recording_ids[2] = 3;
	for (i = 0; i < BUF_LEN; i++) {
		recordings_buf[0][i] = i;
		recordings_buf[4][i] = i;
		recordings_buf[3][i] = i;
	}
	for (i = 0; i < NUM_CHANNELS; i++) {
		recording_offsets[i] = 0;
	}

	num_recordings = 0;
	write_array_dma(recordings_buf[0], 0, DMA1_Channel5, SPI2);
	while (recording_offsets[0] != 128);

	num_recordings = 1;
	write_array_dma(recordings_buf[4], 4, DMA1_Channel5, SPI2);
	while (recording_offsets[4] != 128);

	num_recordings = 2;
	write_array_dma(recordings_buf[3], 3, DMA1_Channel5, SPI2);
	while (recording_offsets[3] != 128);

	recording_offsets[0] = 0;
	recording_offsets[4] = 0;
	recording_offsets[3] = 0;

	zero_arr(recordings_buf[0]);
	zero_arr(recordings_buf[4]);
	zero_arr(recordings_buf[3]);

	num_to_read = 3;
	playback_ids[0] = 0;
	playback_ids[1] = 4;
	playback_ids[2] = 3;

	read_array_dma(recordings_buf[0], 0, DMA1_Channel4, SPI2);
	while (num_read != 3);

	for (i = 0; i < BUF_LEN; i++) {
		if (recordings_buf[0][i] != i
		 || recordings_buf[4][i] != i
		 || recordings_buf[3][i] != i) {
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

	int i;
	// Note: we have 8 kB of memory on the chip

	init_spi();
	init_dma();

	initialize_locations();

	// Matt's main
	setup_gpio_adc();
	setup_timers(4, 1199);
	setup_adc();
	setup_dac();
	// end of Matt's main

	// Cole's main
	setup_keypad();
	setup_timer6();
	setup_led();
	// end of Cole's main

	for (i = 0; i < NUM_CHANNELS; i++) {
		recording_offsets[i] = 0;
	}

	for (i = 0; i < BUF_LEN; i++) {
		output[i] = 0;
	}

#ifdef UNIT_TEST
//	test_array_operation();
	test_dma_array_operation();
	test_address_lookup();
	test_buf_count();
#endif

	keypad_driver();
}
