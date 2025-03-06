#include <stdint.h>
#include "imxrt.h"

#define PIT_BASE 0xFFFFF300
#define PIT_MCR  (*(volatile uint32_t *)(PIT_BASE + 0x00))
#define PIT_LDVAL0  (*(volatile uint32_t *)(PIT_BASE + 0x100))
#define PIT_CVAL0  (*(volatile uint32_t *)(PIT_BASE + 0x104))
#define PIT_TCTRL0 (*(volatile uint32_t *)(PIT_BASE + 0x108))
#define PIT_TFLG0  (*(volatile uint32_t *)(PIT_BASE + 0x10C))

#define LPSPI4_BASE 0x403A8000
#define LPSPI4_CR    (*(volatile uint32_t *)(LPSPI4_BASE + 0x10))
#define LPSPI4_TCR   (*(volatile uint32_t *)(LPSPI4_BASE + 0x08))
#define LPSPI4_TDR   (*(volatile uint32_t *)(LPSPI4_BASE + 0x40))
#define LPSPI4_SR    (*(volatile uint32_t *)(LPSPI4_BASE + 0x14))
#define LPSPI4_CFGR1 (*(volatile uint32_t *)(LPSPI4_BASE + 0x0C))

#define IRQ_PIT 22  // PIT Interrupt Request number
#define AUDIO_BUFFER_SIZE 256

volatile uint16_t audio_buffer[AUDIO_BUFFER_SIZE];
volatile uint32_t buffer_index = 0;

// SPI Initialization
void spi_init() {
    CCM_CCGR1 |= CCM_CCGR1_LPSPI4(CCM_CCGR_ON);  // Enable LPSPI4 clock
    IOMUXC_SW_MUX_CTL_PAD_GPIO_B0_01 = 2; // Set Pin 11 (B0_01) to LPSPI4_MOSI

    LPSPI4_CR = 0;  // Disable SPI before configuring
    LPSPI4_TCR = (11 << 27) |  // 16-bit transfer (for DAC)
                 (1 << 24) |   // Master mode
                 (0 << 22) |   // Clock polarity 0
                 (0 << 21) |   // Clock phase 0
                 (0 << 18) |   // Prescaler divide by 1
                 (0 << 16);    // Frame size 16 bits

    LPSPI4_CFGR1 = (1 << 0);  // Enable master mode
    LPSPI4_CR = (1 << 0);  // Enable LPSPI4
}

// SPI Send Function (Send 16-bit Sample to DAC)
void spi_send(uint16_t data) {
    while (!(LPSPI4_SR & (1 << 1)));  // Wait until TX FIFO is empty
    LPSPI4_TDR = data;  // Send data to DAC
}

// PIT ISR: Send Sample from Audio Buffer
void pit_isr(void) {
    PIT_TFLG0 = 1;  // Clear PIT interrupt flag

    // Send audio sample to DAC via SPI... sawtooth wave
    spi_send(audio_buffer[buffer_index]);

    // Increment buffer index, wrap around
    buffer_index = (buffer_index + 1) % AUDIO_BUFFER_SIZE;
}

// PIT Timer Initialization
void pit_init(uint32_t frequency) {
    PIT_MCR = 0x00;  // Enable PIT module

    uint32_t load_value = (48000000 / frequency) - 1; // Assuming 48MHz system clock
    PIT_LDVAL0 = load_value;

    PIT_TCTRL0 = 0x3;  // Enable Timer and Interrupt
}

// Main Function
int main(void) {
    spi_init();   // Initialize SPI for DAC communication
    pit_init(8000);  // Initialize PIT at 8kHz for audio output

    // Enable PIT interrupt in NVIC
    *(volatile uint32_t *)(0xE000E100) |= (1 << IRQ_PIT);

    while (1) {
        // The PIT ISR will automatically send audio data to the DAC
    }

    return 0;
}

// Reset Handler (Startup Code)
void Reset_Handler(void) {
    main();
    while (1);
}

