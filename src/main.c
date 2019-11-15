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

void check_byte_operation(void) {
	set_mode();

	write_byte(0x69, 0x00);
	write_byte(0x70, 0x01);
	write_byte(0x70, 0x69);

	uint8_t check1;
	uint8_t check2;
	uint8_t check3;

	check1 = read_byte(0x00);
	check2 = read_byte(0x01);
	check3 = read_byte(0x69);
}

void check_array_operation(void) {
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
}

int main(void) {

	init_spi();

	nano_wait(10000);

	check_array_operation();

	for(;;);
}
