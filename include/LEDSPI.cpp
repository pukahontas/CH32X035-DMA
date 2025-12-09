#include "LEDSPI.h"

/**
 * @brief Compile-time helper to convert a 4-bit nibble to WS2812 SPI bit pattern.
 *
 * @param nibble 4-bit value (0..15).
 * @return 12-bit SPI pattern encoding the nibble as WS2812 bits.
 */
constexpr uint32_t _computeWS2812Pattern(uint8_t nibble)
{
    uint32_t bits = 0;
    for (uint8_t bit = 0b1000; bit; bit >>= 1)
    {
        bits <<= BITS_PER_SIGNAL;
        bits |= (nibble & bit) ? SIGNAL_HIGH : SIGNAL_LOW;
    }
    return bits;
}

/**
 * @brief Compile-time generated lookup table for WS2812 bit patterns.
 *
 * Maps 4-bit color nibbles (0..15) to their 32-bit SPI encodings.
 * Table is computed at compile time using constexpr.
 */
constexpr uint32_t WS2812_LUT[16] = {
    _computeWS2812Pattern(0x0),
    _computeWS2812Pattern(0x1),
    _computeWS2812Pattern(0x2),
    _computeWS2812Pattern(0x3),
    _computeWS2812Pattern(0x4),
    _computeWS2812Pattern(0x5),
    _computeWS2812Pattern(0x6),
    _computeWS2812Pattern(0x7),
    _computeWS2812Pattern(0x8),
    _computeWS2812Pattern(0x9),
    _computeWS2812Pattern(0xA),
    _computeWS2812Pattern(0xB),
    _computeWS2812Pattern(0xC),
    _computeWS2812Pattern(0xD),
    _computeWS2812Pattern(0xE),
    _computeWS2812Pattern(0xF),
};

LED_SPI_CH32::LED_SPI_CH32(size_t numLEDs)
    : _numLEDs(numLEDs),
      _LEDColorsSize(numLEDs * 3),
      _DMABufferSize(numLEDs * 3 * BITS_PER_SIGNAL),
      _LEDColors(nullptr),
      _DMABuffer(nullptr)
{
    // Validate LED count
    if (_numLEDs > MAX_SUPPORTED_LEDS)
    {
        return; // Failed validation; allocations will remain null
    }

    // Allocate buffers dynamically
    _LEDColors = new uint8_t[_LEDColorsSize](); // Zero-initialized
    _DMABuffer = new uint8_t[_DMABufferSize](); // Zero-initialized

    // Initialize DMA channel3 (SPI peripheral channel) for writing to the SPI transmit buffer
    // Initialize DMASettings here so this helper owns the DMA configuration.
    _DMASettings.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DATAR);
    _DMASettings.DMA_MemoryBaseAddr = (uint32_t)_DMABuffer;
    _DMASettings.DMA_DIR = DMA_DIR_PeripheralDST;
    _DMASettings.DMA_BufferSize = _DMABufferSize;
    _DMASettings.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    _DMASettings.DMA_MemoryInc = DMA_MemoryInc_Enable;
    _DMASettings.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    _DMASettings.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    _DMASettings.DMA_Mode = DMA_Mode_Normal;
    _DMASettings.DMA_Priority = DMA_Priority_High;
    _DMASettings.DMA_M2M = DMA_M2M_Disable;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_Init(_DMAChannel, &_DMASettings);

    // Initialize the SPI peripheral
    SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE0, SPI_TRANSMITONLY));

    // Set SPI to send DMA request when transmit buffer is empty
    SPI1->CTLR2 |= SPI_CTLR2_TXDMAEN;

    SPI1->CTLR1 &= ~SPI_CTLR1_BR;           // Unset the Timing bits
    SPI1->CTLR1 |= SPI_BaudRatePrescaler_8; // Set prescaler to 8 for 6MHz SPI clock (48MHz / 8 = 6MHz)
}

void LED_SPI_CH32::send()
{
    // Initialize DMASettings here so this helper owns the DMA configuration.

    DMA1->INTFCR = 0xF00; // Clear the flags on channel 3 //Clear the completion flag to indicate a new transfer
    SPI_Cmd(SPI1, ENABLE);
    DMA_Init(_DMAChannel, &_DMASettings);
    DMA_Cmd(_DMAChannel, ENABLE);
}

void LED_SPI_CH32::stop()
{
}

void LED_SPI_CH32::setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= _numLEDs)
        return;

    size_t offset = index * 3;
    size_t dmaIndex = index * 3 * BITS_PER_SIGNAL;

    _LEDColors[offset] = r;
    _LEDColors[offset + 1] = g;
    _LEDColors[offset + 2] = b;

    for (char i = 0; i < 3; i++)
    {
        uint8_t colorChannel = (i == 0) ? g : (i == 1) ? r
                                                       : b; // WS2812 uses GRB order

        // Look up the WS2812 bit patterns for the high and low nibbles from the compile-time table
        uint32_t bitPatternHigh = WS2812_LUT[(colorChannel >> 4) & 0x0F];
        uint32_t bitPatternLow = WS2812_LUT[colorChannel & 0x0F];

        // Assign the 24 bits (3 bytes) of the WS2812 bit pattern for each color channel to the DMA buffer
        for (int i = 0; i < 4; i++)
        {
            _DMABuffer[dmaIndex++] = (bitPatternHigh >> ((3 - i) * BITS_PER_SIGNAL)) & 0xFF;
        }
        for (int i = 0; i < 4; i++)
        {
            _DMABuffer[dmaIndex++] = (bitPatternLow >> ((3 - i) * BITS_PER_SIGNAL)) & 0xFF;
        }
    }
}

void LED_SPI_CH32::clear()
{
    for (uint16_t i = 0; i < _numLEDs; i++)
    {
        setLED(i, 0, 0, 0);
    }
}