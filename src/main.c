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

#include "spi.h"

void nano_wait(unsigned int n) {
    asm(    "        mov r0,%0\n"
            "repeat: sub r0,#83\n"
            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

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

#ifdef UNIT_TEST
	test_array_operation();
	test_address_lookup();
	test_buf_count();
#endif

//	DMA1

	for(;;);
}
