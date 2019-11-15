

void init_spi(void);
void set_mode(void);
void write_byte(uint8_t byte, uint8_t address);
void write_array(uint8_t * array, uint16_t len, uint8_t address);
uint8_t read_byte(uint8_t address);
void read_array(uint8_t * array, uint16_t len, uint8_t address);
