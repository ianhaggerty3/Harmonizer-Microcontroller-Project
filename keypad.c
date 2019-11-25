#include "stm32f0xx.h"
#include "stm32f0_discovery.h"
#include "keypad.h"
#include <stdint.h>
#include <math.h>

int record=0;
int record_ch=0;

int delete=0;
int col = 0;
int8_t history[16] = {0};
int8_t lookup[16] = {1,4,7,0xe,2,5,8,0,3,6,9,0xf,0xa,0xb,0xc,0xd};
char char_lookup[16] = {'1','4','7','*','2','5','8','0','3','6','9','#','A','B','C','D'};


void keypad_driver()
{
    setup_keypad();
    setup_led();
    setup_timer6();

    while(1) {

        char key = get_char_key();
        int channel = get_channel(key);

        ///recording stuff
        if ((key == '*') && record_ch==0) //initiate recording
              record_init(); //sets record=1
        if (channel >= 0 && record==1) //setting ch to record to
            record_select_ch(channel); //sets record_ch=1
        if ((key == '*') && record_ch==1) record_save(channel);

        ///playback stuff
        if (channel >= 0 && record==0)
            playback(channel);

        //delete stuff
        if (key == 'D') {
            delete_init();
        }
        if (channel >= 0 && delete==1)
            delete_select_ch(channel);

        if (key == '#') {
        	go_crazy();
        }

    }
}

/////////////////////////////RECORD START//////////////////
void record_init(){
    //turn on led to signal a "*" button press
    GPIOA->ODR |= 1<<8;
    record=1;
}
void record_select_ch(int ch){
    record_ch = 1;
    pulse_led(ch + 1);
}
void record_save(int ch){
    for (int x=0; x<18; x++)
        nano_wait(100000000);
    GPIOA->ODR &= ~(1<<8);

    //USE MIC TO RECORD AND STORE INTO SPI
    dmarecord(ch);

    //set all params back to idle state
    record=0;
    record_ch=0;
    pulse_led(1);
    GPIOA->ODR &= ~(1<<8);
}
/////////////////////////////RECORD END//////////////////

int get_channel(char ch) {
    int ret_val = -1;

    ret_val = ch == '1' ? 0 : ret_val;
    ret_val = ch == '2' ? 1 : ret_val;
    ret_val = ch == '3' ? 2 : ret_val;
    ret_val = ch == '4' ? 3 : ret_val;
    ret_val = ch == '5' ? 4 : ret_val;
    ret_val = ch == '6' ? 5 : ret_val;
    ret_val = ch == '7' ? 6 : ret_val;
    ret_val = ch == '8' ? 7 : ret_val;
    ret_val = ch == '9' ? 8 : ret_val;
    ret_val = ch == 'A' ? 9 : ret_val;
    ret_val = ch == 'B' ? 10 : ret_val;
    ret_val = ch == 'C' ? 11 : ret_val;

    return ret_val;
}

/////////////////////////////PLAYBACK START//////////////
void playback(int ch) {
    if (get_channel(recording_ids, num_recordings, ch) == -1) return; //returns if not already recorded

    recording_offsets[ch]= 0;
    if (get_channel(playback_ids, num_to_read, ch) == -1) //if audio is currently playing
        return;

    //its not currently playing, add to playback queue
    playback_ids[num_to_read] = ch;
    num_to_read++;

    dmaplayback();
}
/////////////////////////////PLAYBACK END//////////////

/////////////////////////////DELETE START//////////////
void delete_init(){
    pulse_led(1);
    GPIOA->ODR &= ~(1<<8);
    delete=1;
}

void delete_select_ch(int ch){
	int index;

	index = get_channel(recording_ids, num_recordings, ch);
	if (index == -1) return;

	num_recordings--;
	recording_ids[index] = recording_ids[num_recordings];

	index = get_channel(playback_ids, num_to_read, ch);
	if (index == -1) return;

	num_to_read--;
	playback_ids[index] = playback_ids[num_to_read];

    //set param back to idle state
    delete=0;
}
/////////////////////////////DELETE END//////////////

/////////////////////////////CRAZY START//////////////
void go_crazy(void) {
	int i;

	for (i = 0; i < num_recordings; i++) {
		recording_offsets[i] = 0;
		playback_ids[i] = recording_ids[i];
	}
	num_to_read = num_recordings;
}

/////////////////////////////CRAZY END//////////////

void pulse_led(int count){
    for (int x=0; x<count; x++){
        GPIOA->ODR &= ~(1<<8);
        nano_wait(100000000);
        GPIOA->ODR |= 1<<8;
        nano_wait(199990000);
    }
}

int get_key_pressed() { //2
    int key = get_key_press();
    while(key != get_key_release());
    return key;
}

char get_char_key() { //1
    int index = get_key_pressed();
    return char_lookup[index];
}

int get_key_press() { //3 - loops here
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

void setup_led() {
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN; //enable clk to GPIOA
    GPIOA->MODER &= ~(3<<(2*8)); //pa8 clear bits first
    GPIOA->MODER |= 1<<(2*8); //pa8 set bits for output
    //GPIOA->ODR |= 1<<8; //pa8
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

void setup_timer6() {
    RCC->APB1ENR |= RCC_APB1ENR_TIM6EN;
    TIM6->ARR =480-1;
    TIM6->PSC =100-1;
    TIM6->DIER |= TIM_DIER_UIE;
    TIM6->CR1 |= TIM_CR1_CEN;
    NVIC->ISER[0] = 1<<TIM6_DAC_IRQn;
}

void TIM6_DAC_IRQHandler() { //tiggers every 1ms to scan keypad
    TIM6->SR &= ~TIM_SR_UIF; //acknowledge interrupt
    int row = (GPIOB->IDR>>4) &0xf;
    int index = col<<2;
    history[index] = (history[index])<<1;
    history[index] |= (row & 0x1);
    history[index+1] = (history[index+1])<<1;
    history[index+2] = (history[index+2])<<1;
    history[index+3] = (history[index+3])<<1;
    history[index+1] |= ((row>>1) & 0x1);
    history[index+2] |= ((row>>2) & 0x1);
    history[index+3] |= ((row>>3) & 0x1);
    col++;
    if (col>3)
        col=0;
    GPIOB->ODR = (1<<col);
}

void nano_wait(unsigned long n) {
    asm(    "        mov r0,%0\n"

            "repeat: sub r0,#83\n"

            "        bgt repeat\n" : : "r"(n) : "r0", "cc");
}

