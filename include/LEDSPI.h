#pragma once

#include <Arduino.h>
#include <SPI.h>

#define MAX_SUPPORTED_LEDS 300
#define BITS_PER_SIGNAL 3
#define SIGNAL_LOW 0b100
#define SIGNAL_HIGH 0b110

class LED_SPI_CH32
{
public:
    LED_SPI_CH32();
    void init(uint16_t numLEDs);
    void begin();
    void stop();
    void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
    uint8_t* getBuffer();
    void clear();

private:
    uint16_t _numLEDs;
    DMA_Channel_TypeDef* _DMAChannel = DMA1_Channel3;
    SPI_TypeDef* _SPI = SPI1;
    DMA_InitTypeDef _DMASettings;

    static const size_t _LEDColorsSize = MAX_SUPPORTED_LEDS * 3;
    static const size_t _DMABufferSize = MAX_SUPPORTED_LEDS * 3 * BITS_PER_SIGNAL; 
    uint8_t _LEDColors[LED_SPI_CH32::_LEDColorsSize]; 
    uint8_t _DMABuffer[LED_SPI_CH32::_DMABufferSize] = {0b01010101};

    // Look-up table to convert 4-bit color nibble to WS2812 bit pattern
    //static uint16_t _WS2812_COLOR_LUT[16];
};

#include "LEDSPI.cpp"
