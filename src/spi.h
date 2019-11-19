#define MEM_SIZE 131072
#define MEM_SEGMENTS 4
#define BUF_SIZE 128
#define BUF_COUNT ((MEM_SIZE / MEM_SEGMENTS) / BUF_SIZE)
#define NUM_CHANNELS 16

uint8_t recordings_buf1[NUM_CHANNELS][BUF_SIZE];
uint8_t recordings_buf2[NUM_CHANNELS][BUF_SIZE];
uint8_t recording_ids[NUM_CHANNELS];
uint8_t recording_location_and_base_addrs[NUM_CHANNELS];
uint16_t recording_offsets[NUM_CHANNELS];
uint8_t num_recordings;

uint16_t recording_endings[16];

uint8_t device_lookup(uint8_t base);
void address_lookup(uint8_t * address_array, uint16_t offset, uint8_t base);

void init_dma(void);

void init_spi(void);
void write_array(uint8_t * array, uint16_t len, uint8_t address);
void read_array(uint8_t * array, uint16_t len, uint8_t address);
