#include "LEDSPI.h"

LED_SPI_CH32::LED_SPI_CH32(size_t numLEDs, const LED_SPI_CH32_Settings &settings)
    : _numLEDs(numLEDs),
      _LEDColorsSize(numLEDs * 3),
      _DMABufferSize(numLEDs * 3 * BITS_PER_SIGNAL),
      _LEDColors(nullptr),
      _DMAChannel(settings.dmaChannel),
      _SPI(settings.spiInstance)
{
    // Validate LED count
    if (_numLEDs > MAX_SUPPORTED_LEDS)
    {
        return; // Failed validation; allocations will remain null
    }

    // Allocate buffers dynamically
    _LEDColors = new uint8_t[_LEDColorsSize](); // Zero-initialized

    // Allocate multiple DMA buffers for temporal dithering
    for (size_t i = 0; i < NUM_DMA_BUFFERS; i++)
    {
        _DMABuffer[i] = new uint8_t[_DMABufferSize](); // Zero-initialized
    }

    // Initialize DMA channel (SPI peripheral channel) for writing to the SPI transmit buffer
    // Use the first buffer initially
    _DMASettings.DMA_PeripheralBaseAddr = (uint32_t)&(settings.spiInstance->DATAR);
    _DMASettings.DMA_MemoryBaseAddr = (uint32_t)_DMABuffer[_currentBufferIndex];
    _DMASettings.DMA_DIR = DMA_DIR_PeripheralDST;
    _DMASettings.DMA_BufferSize = _DMABufferSize;
    _DMASettings.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    _DMASettings.DMA_MemoryInc = DMA_MemoryInc_Enable;
    _DMASettings.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    _DMASettings.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    _DMASettings.DMA_Mode = DMA_Mode_Normal;
    _DMASettings.DMA_Priority = DMA_Priority_High;
    _DMASettings.DMA_M2M = DMA_M2M_Disable;

    // Initialize DMA channel (SPI peripheral channel) for writing to the SPI transmit buffer during the wait period
    // Send a single zero byte repeatedly without incrementing
    uint8_t* zeroByte = {};
    _DMASettings.DMA_PeripheralBaseAddr = (uint32_t)&(settings.spiInstance->DATAR);
    _DMASettings.DMA_MemoryBaseAddr = (uint32_t)zeroByte;
    _DMASettings.DMA_DIR = DMA_DIR_PeripheralDST;
    _DMASettings.DMA_BufferSize = WAIT_PERIOD_COUNT;
    _DMASettings.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    _DMASettings.DMA_MemoryInc = DMA_MemoryInc_Disable;
    _DMASettings.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
    _DMASettings.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
    _DMASettings.DMA_Mode = DMA_Mode_Normal;
    _DMASettings.DMA_Priority = DMA_Priority_High;
    _DMASettings.DMA_M2M = DMA_M2M_Disable;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
    DMA_Init(_DMAChannel, &_DMASettings);

    // Enable interrupts on transfer complete
    DMA_ITConfig(_DMAChannel, DMA_IT_TC, DISABLE);

    // Initialize the SPI peripheral with settings from the configuration object
    SPI.beginTransaction(SPISettings(settings.spiClockHz, settings.spiDataOrder, settings.spiMode, SPI_TRANSMITONLY));

    // Set SPI to send DMA request when transmit buffer is empty
    settings.spiInstance->CTLR2 |= SPI_CTLR2_TXDMAEN;

    // Set prescaler to 8 for 6MHz SPI clock (48MHz / 8 = 6MHz)
    settings.spiInstance->CTLR1 &= ~SPI_CTLR1_BR; // Unset the Timing bits
    settings.spiInstance->CTLR1 |= SPI_BaudRatePrescaler_8;

    // Register this instance as the singleton for interrupt handler access
    _instance = this;
}

void LED_SPI_CH32::sendColors()
{
    // Initialize DMASettings here so this helper owns the DMA configuration.

    SPI_Cmd(SPI1, ENABLE);
    DMA_Init(_DMAChannel, &_DMASettings);
    DMA_Cmd(_DMAChannel, ENABLE);
    DMA_ClearFlag(DMA1_IT_GL3);
    _isBusy = true;
    _isWaitPeriod = false;
}

void LED_SPI_CH32::sendWait()
{
    // Initialize DMASettings here so this helper owns the DMA configuration.

    SPI_Cmd(SPI1, ENABLE);
    DMA_Init(_DMAChannel, &_DMASettingsWaitPeriod);
    DMA_Cmd(_DMAChannel, ENABLE);
    DMA_ClearFlag(DMA1_IT_GL3);
    _isBusy = true;
    _isWaitPeriod = true;
}

void LED_SPI_CH32::stop()
{
}

void LED_SPI_CH32::setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    // Use the current buffer index
    for (size_t i = 0; i < NUM_DMA_BUFFERS; i++)
    {
        setLED(index, r, g, b, i);
    }
}

void LED_SPI_CH32::setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b, uint8_t bufferIndex)
{
    if (index >= _numLEDs || bufferIndex >= NUM_DMA_BUFFERS)
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

        // Assign the bit pattern for each color channel to the specified DMA buffer
        size_t bufDmaIndex = dmaIndex;
        for (int i = 0; i < 4; i++)
        {
            _DMABuffer[bufferIndex][bufDmaIndex++] = (bitPatternHigh >> ((3 - i) * BITS_PER_SIGNAL)) & 0xFF;
        }
        for (int i = 0; i < 4; i++)
        {
            _DMABuffer[bufferIndex][bufDmaIndex++] = (bitPatternLow >> ((3 - i) * BITS_PER_SIGNAL)) & 0xFF;
        }
    }
}

void LED_SPI_CH32::setLED(uint16_t index, float r, float g, float b)
{
    setLED(index, (uint8_t)(r * 255), (uint8_t)(g * 255), (uint8_t)(b * 255));
}

void LED_SPI_CH32::clear()
{
    for (uint16_t i = 0; i < _numLEDs; i++)
    {
        setLED(i, (uint8_t)0, (uint8_t)0, (uint8_t)0);
    }
}

/**
 * @brief Swap to the next DMA buffer for temporal dithering.
 *
 * Cycles through available buffers (0 to NUM_DMA_BUFFERS-1) and updates
 * the DMA settings to use the new buffer on the next transfer.
 */
void LED_SPI_CH32::swapDMABuffer()
{
    // Advance to the next buffer index (circular)
    _currentBufferIndex = (_currentBufferIndex + 1) % NUM_DMA_BUFFERS;

    // Update DMA settings to point to the new buffer
    _DMASettings.DMA_MemoryBaseAddr = (uint32_t)_DMABuffer[_currentBufferIndex];
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

/**
 * @brief DMA1 Channel 3 interrupt handler for WS2812 DMA transfer completion.
 *
 * Automatically restarts the DMA transfer when the previous transfer completes,
 * enabling continuous LED refresh without software intervention.
 */
void LED_SPI_CH32::interruptHandler()
{
    return;
    // Check if this is a Transfer Complete (TC) interrupt
    if (DMA1->INTFR & DMA1_IT_TC3)
    {
        // Clear all interrupt flags on channel 3
        DMA1->INTFCR = DMA1_IT_GL3;

        // Restart the DMA transfer if the singleton instance exists
        if (getInstance())
        {
            getInstance()->_isWaitPeriod ?
                getInstance()->sendColors():
                getInstance()->sendWait();
        }
    }
}

/// Singleton instance pointer definition.
LED_SPI_CH32 *LED_SPI_CH32::_instance = nullptr;

/**
 * @brief DMA1 Channel 3 interrupt handler for WS2812 DMA transfer completion.
 *
 * Automatically restarts the DMA transfer when the previous transfer completes,
 * enabling continuous LED refresh without software intervention.
 */
// Global interrupt handler that delegates to the static method
/*extern "C" void DMA1_Channel3_IRQHandler(void)
{
    LED_SPI_CH32::interruptHandler();
}*/

/* TODO:
Fix IRQ
Set up LUT to work with different singnal bit widths
Break out processor specific settings into their own function
Breake out LED type specific settings into their own function
*/