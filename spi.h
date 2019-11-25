#define MEM_SIZE 131072
#define MEM_SEGMENTS 4 
#define BUF_LEN 128
#define BUF_COUNT ((MEM_SIZE / MEM_SEGMENTS) / BUF_LEN)
#define NUM_CHANNELS 12

uint8_t recordings_buf1[NUM_CHANNELS][BUF_LEN];
uint8_t recordings_buf2[NUM_CHANNELS][BUF_LEN];
uint8_t recording_ids[NUM_CHANNELS];
uint8_t recording_location_and_base_addrs[NUM_CHANNELS];
uint16_t recording_offsets[NUM_CHANNELS];
volatile uint8_t num_recordings;
volatile uint8_t num_read;

uint8_t device_lookup(uint8_t base);
void address_lookup(uint8_t * address_array, uint8_t base, uint16_t offset);

void init_dma(void);

void init_spi(void);
void write_array(uint8_t * array, uint16_t len, uint8_t address);
void read_array(uint8_t * array, uint16_t len, uint8_t address);

void write_array_dma(uint8_t * array, uint8_t id, DMA_Channel_TypeDef * dma_channel, SPI_TypeDef * spi);
void read_array_dma(uint8_t * array, uint8_t id, DMA_Channel_TypeDef * dma_channel, SPI_TypeDef * spi);
