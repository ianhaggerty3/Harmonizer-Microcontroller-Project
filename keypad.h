#ifndef KEYPAD_H
#define KEYPAD_H

void keypad_driver(void);
void setup_keypad(void);
void setup_led(void);
int get_key_pressed(void);
char get_char_key(void);
int get_key_press(void);
int get_key_release(void);
void setup_timer6(void);

void reset_state(void);
void record_init(void);
void record_save(int);

void continuous_init(void);
void set_continuous(int);

void playback(int);
void go_crazy(void);

void delete_init();
void delete_select_ch(int ch);
int get_channel(char ch);

void pulse_led(int);

#endif
