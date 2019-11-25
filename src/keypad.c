#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "keypad.h"
#include <stdint.h>
#include <math.h>

int col = 0;
int8_t history[16] = {0};
int8_t lookup[16] = {1,4,7,0xe,2,5,8,0,3,6,9,0xf,0xa,0xb,0xc,0xd};
char char_lookup[16] = {'1','4','7','*','2','5','8','0','3','6','9','#','A','B','C','D'};


void driver()
{
    setup_keypad();
    setup_gpio();

    while(1) {

        char key = get_char_key();
        if (key == '*') //check if * is pressed
        {
            GPIOA->ODR |= 1<<8; //pa8
            GPIOA->ODR |= 1<<9; //pa9
            GPIOA->ODR |= 1<<10; //pa10
        }
        if(key == '#') //check if # is pressed
        {
            //do other stuff
        }
    }
}

int get_key_pressed() {
    int key = get_key_press();
    while(key != get_key_release());
    return key;
}

char get_char_key() {
    int index = get_key_pressed();
    return char_lookup[index];
}

int get_key_press() {
    while (1)
    {
        for (int i=0; i<16; i++)
        {
            if (history[i] == 1)
                return i;
        }
    }
}

int get_key_release() {
    while (1)
    {
        for (int i=0; i<16; i++)
        {
            if (history[i] == -2)
                return i;
        }
    }
}

void setup_gpio() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; //enable clk to GPIOA
    //set PA8(CH1) PA9(CH2) PA10(CH3) to alternative fn mode (10)
    //PA8-PA10 use AFRH_AFR2 w/ code 0010
    GPIOA->MODER &= ~(0xf<<(2*8)); //pa8 clear all bits first
    GPIOA->MODER |= 2<<(2*8); //pa8 set bits
    GPIOA->AFR[1] &= ~(0xf << (4*(8-8))); //clear all bits first
    GPIOA->AFR[1] |=   0x2 << (4*(8-8)); //set bits

    GPIOA->MODER &= ~(0xf<<(2*9)); //pa9 clear all bits frist
    GPIOA->MODER |= 2<<(2*9); //pa9 set bits
    GPIOA->AFR[1] &= ~(0xf << (4*(9-8))); //clear all bits first
    GPIOA->AFR[1] |=   0x2 << (4*(9-8)); //set bits

    GPIOA->MODER &= ~(0xf<<(2*10)); //pa10 clear all bits first
    GPIOA->MODER |= 2<<(2*10); //pa10 set bits
    GPIOA->AFR[1] &= ~(0xf << (4*(10-8))); //clear all bits first
    GPIOA->AFR[1] |=   0x2 << (4*(10-8)); //set bits
}

void setup_keypad() {
    RCC->AHBENR |= RCC_AHBENR_GPIOBEN; //enable clk to GPIOA
    //set 0,1,2,3 as outputs(01)
    GPIOB->MODER &= ~(0xf<<(2*0)); //clear all
    GPIOB->MODER |= 1<<(2*0); //set

    GPIOB->MODER &= ~(0xf<<(2*1)); //clear all
    GPIOB->MODER |= 1<<(2*1); //set

    GPIOB->MODER &= ~(0xf<<(2*2)); //clear all
    GPIOB->MODER |= 1<<(2*2); //set

    GPIOB->MODER &= ~(0xf<<(2*3)); //clear all
    GPIOB->MODER |= 1<<(2*3); //set

    //set 4,5,6,7 as inputs(00) and pull down resistor(10)
    GPIOB->MODER &= ~(0xf<<(2*4)); //clear all
    GPIOB->PUPDR &= ~(0xf<<(2*4)); //clear all
    GPIOB->PUPDR |= 2<<(2*4); //set

    GPIOB->MODER &= ~(0xf<<(2*5)); //clear all
    GPIOB->PUPDR &= ~(0xf<<(2*5)); //clear all
    GPIOB->PUPDR |= 2<<(2*5); //set

    GPIOB->MODER &= ~(0xf<<(2*6)); //clear all
    GPIOB->PUPDR &= ~(0xf<<(2*6)); //clear all
    GPIOB->PUPDR |= 2<<(2*6); //set

    GPIOB->MODER &= ~(0xf<<(2*7)); //clear all
    GPIOB->PUPDR &= ~(0xf<<(2*7)); //clear all
    GPIOB->PUPDR |= 2<<(2*7); //set
}
