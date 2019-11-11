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

int main(void)
{

	nano_wait(1000);

	init_spi();

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

	for(;;);
}
