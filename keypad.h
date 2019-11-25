
void keypad_driver(void);
void setup_keypad(void);
void setup_led(void);
int get_key_pressed(void);
char get_char_key(void);
int get_key_press(void);
int get_key_release(void);
void setup_timer6(void);
void nano_wait(unsigned long n);

void record_init(void);
void record_select_ch(char);
void record_save(void);

void playback(char);

void delete_init();
void delete_select_ch(char);

void pulse_led(int);
