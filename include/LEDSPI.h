#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <cstddef>
#include <cstdint>

#define MAX_SUPPORTED_LEDS 300
#define BITS_PER_SIGNAL 8
#define SIGNAL_LOW 0b11000000
#define SIGNAL_HIGH 0b11111000

/**
 * @class LED_SPI_CH32
 * @brief WS2812 LED driver using SPI + DMA on CH32X035.
 *
 * Controls addressable RGB LEDs (WS2812/NeoPixel) via SPI with DMA transfers.
 * Buffers are dynamically allocated based on the number of LEDs.
 */
class LED_SPI_CH32
{
public:
    /**
     * @fn LED_SPI_CH32(size_t numLEDs)
     * @brief Construct an LED controller and allocate buffers for the given number of LEDs.
     *
     * @param numLEDs Number of addressable LEDs to control (must be <= MAX_SUPPORTED_LEDS).
     */
    explicit LED_SPI_CH32(size_t numLEDs);

    /**
     * @fn void begin()
     * @brief Start a DMA transfer of the LED color buffer.
     */
    void send();

    /**
     * @fn void stop()
     * @brief Stop ongoing SPI/DMA transfers.
     */
    void stop();

    /**
     * @fn void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
     * @brief Set the color of an LED.
     *
     * @param index LED index (0 to numLEDs-1).
     * @param r Red component (0..255).
     * @param g Green component (0..255).
     * @param b Blue component (0..255).
     */
    void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b);

    /**
     * @fn void clear()
     * @brief Clear all LED colors to black (0, 0, 0).
     */
    void clear();

//private:
    const size_t _numLEDs;
    const size_t _LEDColorsSize;
    const size_t _DMABufferSize;

    DMA_Channel_TypeDef* _DMAChannel = DMA1_Channel3;
    SPI_TypeDef* _SPI = SPI1;
    DMA_InitTypeDef _DMASettings;

    uint8_t* _LEDColors;     ///< Dynamically allocated RGB color buffer.
    uint8_t* _DMABuffer;     ///< Dynamically allocated DMA/SPI bit pattern buffer.
};

#include "LEDSPI.cpp"

