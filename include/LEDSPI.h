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
    LED_SPI_CH32(size_t numLEDs);
    void init();
    void begin();
    void stop();
    void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
    uint8_t* getBuffer();
    void clear();

//private:
    const size_t _numLEDs;
    DMA_Channel_TypeDef* _DMAChannel = DMA1_Channel3;
    SPI_TypeDef* _SPI = SPI1;
    DMA_InitTypeDef _DMASettings;

    const size_t _LEDColorsSize = _numLEDs * 3;
    const size_t _DMABufferSize = _numLEDs * 3 * BITS_PER_SIGNAL; 
    uint8_t _LEDColors[MAX_SUPPORTED_LEDS * 3]; 
    uint8_t _DMABuffer[MAX_SUPPORTED_LEDS * 3 * BITS_PER_SIGNAL];

    // Look-up table to convert 4-bit color nibble to WS2812 bit pattern
    //static uint16_t _WS2812_COLOR_LUT[16];
};

#include "LEDSPI.cpp"
