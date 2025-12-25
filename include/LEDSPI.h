#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <cstddef>
#include <cstdint>

#define MAX_SUPPORTED_LEDS 300
#define BITS_PER_SIGNAL 8
#define SIGNAL_LOW 0b11000000
#define SIGNAL_HIGH 0b11111000
#define NUM_DMA_BUFFERS 1  ///< Number of DMA buffers for temporal dithering
#define WAIT_PERIOD_COUNT 38 // 38 bytes to wait 50 us = (50 us) / (8 bits) * (48 MHz / 8 prescaler)

/**
 * @struct LED_SPI_CH32_Settings
 * @brief Configuration settings for LED_SPI_CH32 driver.
 *
 * Allows customization of SPI clock speed, DMA channel, and peripheral instances.
 */
struct LED_SPI_CH32_Settings
{
    /// SPI clock speed in Hz (default: 6,000,000 = 6 MHz)
    uint32_t spiClockHz = 6000000;

    /// SPI data order (default: MSBFIRST for WS2812 protocol)
    BitOrder spiDataOrder = MSBFIRST;

    /// SPI mode (default: SPI_MODE0)
    uint8_t spiMode = SPI_MODE0;

    /// DMA channel to use (default: DMA1_Channel3)
    DMA_Channel_TypeDef* dmaChannel = DMA1_Channel3;

    /// SPI peripheral instance (default: SPI1)
    SPI_TypeDef* spiInstance = SPI1;
};

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
     * @fn LED_SPI_CH32(size_t numLEDs, const LED_SPI_CH32_Settings& settings)
     * @brief Construct an LED controller and allocate buffers for the given number of LEDs.
     *
     * @param numLEDs Number of addressable LEDs to control (must be <= MAX_SUPPORTED_LEDS).
     * @param settings Configuration settings for SPI, DMA, and peripherals (uses defaults if not specified).
     */
    explicit LED_SPI_CH32(size_t numLEDs, const LED_SPI_CH32_Settings& settings = LED_SPI_CH32_Settings());

    /**
     * @fn void begin()
     * @brief Start a DMA transfer of the LED color buffer.
     */
    void sendColors();

    /**
     * @fn void begin()
     * @brief Start a DMA transfer of the wait signal, all zeros for the reset time
     */
    void sendWait();

    /**
     * @fn void stop()
     * @brief Stop ongoing SPI/DMA transfers.
     */
    void stop();

    /**
     * @fn void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
     * @brief Set the color of an LED in the current buffer.
     *
     * @param index LED index (0 to numLEDs-1).
     * @param r Red component (0..255).
     * @param g Green component (0..255).
     * @param b Blue component (0..255).
     */
    void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b);
    
    /**
     * @fn void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t bufferIndex)
     * @brief Set the color of an LED in a specific buffer (for temporal dithering).
     *
     * @param index LED index (0 to numLEDs-1).
     * @param r Red component (0..255).
     * @param g Green component (0..255).
     * @param b Blue component (0..255).
     * @param bufferIndex DMA buffer index (0 to NUM_DMA_BUFFERS-1).
     */
    void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t bufferIndex);
    
    void setLED(uint16_t index, float r, float g, float b);
    
    /**
     * @fn void setLED(uint16_t index, float r, float g, float b, uint8_t bufferIndex)
     * @brief Set the color of an LED with float components in a specific buffer.
     *
     * @param index LED index (0 to numLEDs-1).
     * @param r Red component (0.0..1.0).
     * @param g Green component (0.0..1.0).
     * @param b Blue component (0.0..1.0).
     * @param bufferIndex DMA buffer index (0 to NUM_DMA_BUFFERS-1).
     */
    void setLED(uint16_t index, float r, float g, float b, uint8_t bufferIndex);

    /**
     * @fn void clear()
     * @brief Clear all LED colors to black (0, 0, 0).
     */
    void clear();

//private:
    const size_t _numLEDs;
    const size_t _LEDColorsSize;
    const size_t _DMABufferSize;

    DMA_Channel_TypeDef* _DMAChannel;
    SPI_TypeDef* _SPI;
    DMA_InitTypeDef _DMASettings;
    DMA_InitTypeDef _DMASettingsWaitPeriod;

    uint8_t* _LEDColors;     ///< Dynamically allocated RGB color buffer.
    uint8_t* _DMABuffer[NUM_DMA_BUFFERS];  ///< Array of DMA buffers for temporal dithering.
    uint8_t _currentBufferIndex = 0;       ///< Index of the currently active buffer.
    bool _isBusy = false; // Is the SPI currently sending data
    bool _isWaitPeriod = true; // Is the SPI currently sending color data or in the wait period

    /**
     * @fn static LED_SPI_CH32* getInstance()
     * @brief Get the singleton instance for interrupt handler access.
     *
     * @return Pointer to the singleton instance.
     */
    static LED_SPI_CH32* getInstance() { return _instance; }
    bool busy();
    
    /**
     * @fn void swapDMABuffer()
     * @brief Swap to the next DMA buffer for temporal dithering.
     *
     * Cycles through available buffers to enable dithering patterns.
     */
    void swapDMABuffer();
    
    /**
     * @fn uint8_t getCurrentBufferIndex()
     * @brief Get the index of the currently active DMA buffer.
     *
     * @return Current buffer index (0 to NUM_DMA_BUFFERS-1).
     */
    uint8_t getCurrentBufferIndex() const { return _currentBufferIndex; }

    static void interruptHandler();

    /// Singleton instance pointer for interrupt handler access.
    static LED_SPI_CH32* _instance;
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



