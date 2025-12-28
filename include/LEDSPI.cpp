#include "LEDSPI.h"

LED_SPI_CH32::LED_SPI_CH32(size_t numLEDs, uint8_t ditherDepth = 0)
    : _numLEDs(numLEDs),
      _LEDColorsSize(numLEDs * 3),
      _DMABufferSize(numLEDs * 3 * BITS_PER_SIGNAL),
      _LEDColors(nullptr),
      _DMABuffer(nullptr),
      _numDitherBuffers(ditherDepth + 1)
{
    // Validate inputs
    if (_numLEDs > MAX_SUPPORTED_LEDS)
    {
        return; // Failed validation; allocations will remain null
    }

    // Allocate buffers dynamically
    _LEDColors = new uint32_t[_LEDColorsSize]();               // Zero-initialized
    _DMABuffer = new uint8_t[_DMABufferSize * _numDitherBuffers](); // Zero-initialized
    ZERO = new uint8_t[1]();

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

    _DMASettingsWaitPeriod.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DATAR);
    _DMASettingsWaitPeriod.DMA_MemoryBaseAddr = (uint32_t)ZERO;
    _DMASettingsWaitPeriod.DMA_DIR = DMA_DIR_PeripheralDST;
    _DMASettingsWaitPeriod.DMA_BufferSize = WAIT_PERIOD_COUNT;
    _DMASettingsWaitPeriod.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    _DMASettingsWaitPeriod.DMA_MemoryInc = DMA_MemoryInc_Disable;
    _DMASettingsWaitPeriod.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    _DMASettingsWaitPeriod.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    _DMASettingsWaitPeriod.DMA_Mode = DMA_Mode_Normal;
    _DMASettingsWaitPeriod.DMA_Priority = DMA_Priority_High;
    _DMASettingsWaitPeriod.DMA_M2M = DMA_M2M_Disable;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    // Enable interrupts on transfer complete
    DMA_ITConfig(_DMAChannel, DMA_IT_TC, ENABLE);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);

    // Initialize the SPI peripheral
    SPI.beginTransaction(SPISettings(6000000, MSBFIRST, SPI_MODE0, SPI_TRANSMITONLY));

    // Set SPI to send DMA request when transmit buffer is empty
    SPI1->CTLR2 |= SPI_CTLR2_TXDMAEN;

    // Set prescaler to 8 for 6MHz SPI clock (48MHz / 8 = 6MHz)
    SPI1->CTLR1 &= ~SPI_CTLR1_BR; // Unset the Timing bits
    SPI1->CTLR1 |= SPI_BaudRatePrescaler_8;

    // Register this instance as the singleton for interrupt handler access
    _instance = this;
}

void LED_SPI_CH32::send(DMA_InitTypeDef DMASettings)
{
    // Initialize DMASettings here so this helper owns the DMA configuration.

    SPI_Cmd(SPI1, ENABLE);
    DMA_Cmd(_DMAChannel, DISABLE);
    DMA_Init(_DMAChannel, &DMASettings);
    DMA_Cmd(_DMAChannel, ENABLE);
    DMA_ClearFlag(DMA1_IT_GL3);
    _isBusy = true;
}

void LED_SPI_CH32::sendColors()
{
    // Select the buffer to sisplay based on temporal dithering
    // The buffer selected should be the position of the highest set bit (LSB = 0)
    // i.e. if currentBuffer = 5 = 0b0101, the buffer displayed is 2 because the highest bit set is 2^2. 
    uint8_t currentBuffer = 0;
    while (_ditherCounter >> (currentBuffer + 1)) currentBuffer++;

    _DMASettings.DMA_MemoryBaseAddr = (uint32_t)(_DMABuffer + currentBuffer * _DMABufferSize);

    // Increment ditherCounter and clamp it to the range 1..(2^numBuffers-1)
    _ditherCounter++;
    if (_ditherCounter >= (1 << _numDitherBuffers)) _ditherCounter = 1;


    send(_DMASettings);
    _sendWait = true;
}

void LED_SPI_CH32::sendWait()
{
    send(_DMASettingsWaitPeriod);
    _sendWait = false;
}

void LED_SPI_CH32::start()
{
    _start = true;
    sendWait();
}

void LED_SPI_CH32::stop()
{
    _start = false;
}

void LED_SPI_CH32::setLED(uint16_t index, float r, float g, float b)
{
    if (index >= _numLEDs)
        return;

    size_t offset = index * 3;

    //_LEDColors[offset] = r;
    //_LEDColors[offset + 1] = g;
    //_LEDColors[offset + 2] = b;

    size_t dmaIndex = index * 3 * BITS_PER_SIGNAL;

    for (uint8_t i = 0; i < 3; i++)
    {
        float colorChannel = (i == 0) ? g : (i == 1) ? r
                                                     : b; // WS2812 uses GRB order

        uint8_t denominator = (1 << _numDitherBuffers) - 1; // 2^(numBuffers) - 1, the smallest representable fraction of an integer
        uint8_t colorInteger = colorChannel * MAX_BRIGHTNESS;
        uint8_t colorFractional = (colorChannel * MAX_BRIGHTNESS - colorInteger) * denominator + 0.5; // Round to nearest fractional part

        _LEDColors[offset + i] = colorInteger;

        for (size_t ditherBuffer = 0; ditherBuffer < _numDitherBuffers; ditherBuffer++)
        {
            uint8_t colorValue = colorInteger + (colorFractional & (1 << ditherBuffer) ? 1 : 0);

            // Look up the WS2812 bit patterns for the high and low nibbles from the compile-time table
            uint32_t bitPatternHigh = WS2812_LUT[(colorValue >> 4) & 0x0F];
            uint32_t bitPatternLow = WS2812_LUT[colorValue & 0x0F];

            _LEDColors[offset + i] = bitPatternLow;

            // Assign the 24 bits (3 bytes) of the WS2812 bit pattern for each color channel to the DMA buffer
            for (int i = 0; i < 4; i++)
            {
                _DMABuffer[dmaIndex + i + ditherBuffer * _DMABufferSize] = (bitPatternHigh >> ((3 - i) * BITS_PER_SIGNAL)) & 0xFF;
            }
            for (int i = 0; i < 4; i++)
            {
                _DMABuffer[dmaIndex + i + 4 + ditherBuffer * _DMABufferSize] = (bitPatternLow >> ((3 - i) * BITS_PER_SIGNAL)) & 0xFF;
            }
        }
        dmaIndex += 8;
    }
}

void LED_SPI_CH32::clear()
{
    for (uint16_t i = 0; i < _numLEDs; i++)
    {
        setLED(i, 0, 0, 0);
    }
}

void LED_SPI_CH32::handleDMAInterrupt(void)
{
    // Check if this is a Transfer Complete (TC) interrupt
    if (DMA1->INTFR & DMA1_IT_TC3)
    {
        // Clear all interrupt flags on channel 3
        DMA1->INTFCR = DMA1_IT_GL3;

        // If the instance has been stopped, don't restart
        if (!_start)
            return;

        // Restart the DMA transfer
        if (!_sendWait)
        {
            sendColors();
        }
        else
        {
            sendWait();
        }
    }
}

/**
 * @brief Busy wait until the DMA1 Channel 3 interrupt flag is clear.
 *
 * Polls the DMA1 INTFR register to detect when the Transfer Complete
 * interrupt flag has been cleared, typically by the interrupt handler.
 * This is a blocking operation useful for synchronization.
 *
 * @return Number of loop iterations performed (for debugging/profiling).
 */
bool LED_SPI_CH32::busy()
{
    _isBusy &= (DMA_GetFlagStatus(DMA1_FLAG_TC3) == RESET);
    return _isBusy;
}

/// Singleton instance pointer definition.
LED_SPI_CH32 *LED_SPI_CH32::_instance = nullptr;

/**
 * @brief DMA1 Channel 3 interrupt handler for WS2812 DMA transfer completion.
 *
 * Automatically restarts the DMA transfer when the previous transfer completes,
 * enabling continuous LED refresh without software intervention.
 */

extern "C"
{
    void DMA1_Channel3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
    void DMA1_Channel3_IRQHandler(void)
    {
        if (LED_SPI_CH32::getInstance())
        {
            LED_SPI_CH32::getInstance()->handleDMAInterrupt();
        }
    }
}

/* TODO:
Fix IRQ
Set up LUT to work with different singnal bit widths
Break out processor specific settings into their own function
Breake out LED type specific settings into their own function
*/