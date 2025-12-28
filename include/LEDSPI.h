#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <cstddef>
#include <cstdint>

#define MAX_SUPPORTED_LEDS 300
#define BITS_PER_SIGNAL 8
#define SIGNAL_LOW 0b11000000
#define SIGNAL_HIGH 0b11111000
#define WAIT_PERIOD_COUNT 10 // Delay to get to 50us wait time to send the reset signal. There is about 40us of overhead delay
#define MAX_BRIGHTNESS 4


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
    explicit LED_SPI_CH32(size_t numLEDs, uint8_t ditherDepth = 0);

    /**
     * @fn void start()
     * @brief Start sending color data to the LEDs using DMA+SPI. DMA complete interrupts will restart the transaction indefinitely.
     */
    void start();

    /**
     * @fn void stop()
     * @brief Stop ongoing SPI/DMA transfers. Not implemented.
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
    void setLED(uint16_t index, float r, float g, float b);

    /**
     * @fn void clear()
     * @brief Clear all LED colors to black (0, 0, 0).
     */
    void clear();

    void handleDMAInterrupt(void);

    /**
     * @fn static LED_SPI_CH32* getInstance()
     * @brief Get the singleton instance for interrupt handler access.
     *
     * @return Pointer to the singleton instance.
     */
    static LED_SPI_CH32* getInstance() { return _instance; }
    
    bool busy();

//private:
    const size_t _numLEDs;
    const size_t _LEDColorsSize;
    const size_t _DMABufferSize;
    const size_t _numDitherBuffers;

    DMA_Channel_TypeDef* _DMAChannel = DMA1_Channel3;
    SPI_TypeDef* _SPI = SPI1;
    DMA_InitTypeDef _DMASettings;
    DMA_InitTypeDef _DMASettingsWaitPeriod;

    uint32_t* _LEDColors;     ///< Dynamically allocated RGB color buffer.
    uint8_t* _DMABuffer;     ///< Dynamically allocated DMA/SPI bit pattern buffer.
    uint8_t* ZERO;
    bool _start = false;
    bool _isBusy = false;
    bool _sendWait = false;
    uint8_t _ditherCounter = 1;

    /// Singleton instance pointer for interrupt handler access.
    static LED_SPI_CH32* _instance;

    /**
     * @fn void send()
     * @brief Start a DMA transfer using the settings provided
     */
    void send(DMA_InitTypeDef DMASettings);

    void sendColors();

    void sendWait();
};

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

#include "LEDSPI.cpp"



