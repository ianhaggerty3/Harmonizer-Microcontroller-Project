#ifndef SPI_H
#define SPI_H

#define MEM_SIZE 131072
#define MEM_SEGMENTS 4
#define BUF_LEN 128
#define CHANNEL_BYTES MEM_SIZE / MEM_SEGMENTS
#define BUF_COUNT (CHANNEL_BYTES / BUF_LEN)
#define NUM_CHANNELS 12

uint16_t buffers_transmitted;
uint16_t queues_read;

uint8_t recordings_buf[NUM_CHANNELS][BUF_LEN];
uint8_t output[BUF_LEN];
uint8_t recording_ids[NUM_CHANNELS];
uint8_t recording_locations[NUM_CHANNELS];
uint16_t recording_offsets[NUM_CHANNELS];
volatile uint8_t num_recordings;

uint8_t playback_ids[NUM_CHANNELS];
volatile uint8_t num_to_read;
volatile uint8_t num_read;

void nano_wait(unsigned long n);
int lookup_id(uint8_t * arr, int len, int id);
uint8_t device_lookup(uint8_t base);
void address_lookup(uint8_t * address_array, uint8_t base, uint16_t offset);

void init_dma(void);

void init_spi(void);

void write_array_dma(uint8_t * array, uint8_t id, DMA_Channel_TypeDef * dma_channel, SPI_TypeDef * spi);
void read_array_dma(uint8_t * array, uint8_t id, DMA_Channel_TypeDef * dma_channel, SPI_TypeDef * spi);

void initialize_locations(void);

#endif
