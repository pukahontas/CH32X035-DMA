#include "LEDSPI.h"

uint16_t WS2812BitPattern(uint8_t nibble)
{
    nibble &= 0x0F; // Ensure only lower 4 bits are used
    uint16_t bits = 0;
    for (uint8_t bit = 0b1000; bit; bit >>= 1)
    {
        bits <<= BITS_PER_SIGNAL;
        bits |= (nibble & bit) ? SIGNAL_HIGH : SIGNAL_LOW;
    }
    return bits;
}

LED_SPI_CH32::LED_SPI_CH32(size_t numLEDs) : _numLEDs(numLEDs)
{
}

void LED_SPI_CH32::init()
{

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
    SPI.beginTransaction(SPISettings(4800000, MSBFIRST, SPI_MODE0, SPI_TRANSMITONLY));

    // Set SPI to send DMA request when transmit buffer is empty
    SPI1->CTLR2 |= SPI_CTLR2_TXDMAEN;
}

void LED_SPI_CH32::begin()
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

        // Lookup the WS2812 bit pattern for the high and low nibbles and combine them
        uint32_t ws2812BitPattern = WS2812BitPattern((colorChannel >> 4) & 0x0F) << 12 |
                                    WS2812BitPattern(colorChannel & 0x0F);

        // Assign the 24 bits (3 bytes) of the WS2812 bit pattern for each color channel to the DMA buffer
        _DMABuffer[dmaIndex] = (ws2812BitPattern >> 16) & 0xFF;
        _DMABuffer[dmaIndex++] = (ws2812BitPattern >> 8) & 0xFF;
        _DMABuffer[dmaIndex++] = ws2812BitPattern & 0xFF;
    }
}

void LED_SPI_CH32::clear()
{
    for (uint16_t i = 0; i < _numLEDs; i++)
    {
        setLED(i, 0, 0, 0);
    }
}

uint8_t *LED_SPI_CH32::getBuffer()
{
    return _DMABuffer;
}
