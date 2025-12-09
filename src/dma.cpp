#include <Arduino.h>
#include <SPI.h>

class DMA_TIM_Config
{
public:
    DMA_TIM_Config init(DMA_Channel_TypeDef *DMAChannel, uint8_t pinName, uint32_t *DMABuffer, TIM_TypeDef *timPeripheral, uint8_t timChannel, uint32_t targetFreqHz)
    {
        this->DMAChannel = DMAChannel;
        configureDMA(DMAChannel, pinName, DMABuffer);
        configureTimer(timPeripheral, timChannel, targetFreqHz);
        return *this;
    }

    void begin() {
        DMA_Cmd(DMAChannel, ENABLE);

    }

    void setBuffer(uint32_t *DMABuffer)
    {
        // Update the DMA memory address to point to the new buffer
        DMASettings.DMA_MemoryBaseAddr = (uint32_t)DMABuffer;
        DMASettings.DMA_BufferSize = sizeof(DMABuffer);
        DMA_Init(DMAChannel, &DMASettings);
    }

    uint32_t pinSetValue() {
        return setValue;
    }

    uint32_t pinResetValue() {
        return resetValue;
    }

private:
    DMA_Channel_TypeDef *DMAChannel;
    DMA_InitTypeDef DMASettings;
    TIM_TypeDef *TIMPeripheral;
    uint32_t setValue, resetValue;

    /**
     * @fn configureTimer(TIM_TypeDef* TIMx, uint8_t channel, uint32_t targetFreqHz)
     * @brief Configure timer compare events to generate DMA requests.
     *
     * Configure the timer base (PSC/ARR) so that the selected compare channel
     * generates events at approximately targetFreqHz (frequency in Hz). The
     * compare channel is configured but the OC GPIO output is not enabled — this
     * function only sets up compare events so the timer can trigger DMA on CCx.
     *
     * @param TIMx Pointer to the TIM peripheral (e.g., TIM2).
     * @param channel Compare channel number (1..4) to configure for DMA triggers.
     * @param targetFreqHz Target compare event frequency, in Hertz (Hz).
     * @return void
     *
     * @note Uses SystemCoreClock as the timer input clock. If the actual timer
     *       input clock differs (APB prescalers or other clocking), compute the
     *       proper timerClock before calling or modify this function accordingly.
     */
    void configureTimer(TIM_TypeDef *TIMx, uint8_t channel, uint32_t targetFreqHz)
    {
        // Enable timer clock for known timers (add more cases if you use other timers)
        if (TIMx == TIM2)
        {
            RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
        }
        else if (TIMx == TIM1)
        {
            RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
        }

        TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure = {0};

        uint32_t timerClock = SystemCoreClock;

        uint32_t bestPSC = 0;
        uint32_t bestARR = 0;
        for (uint32_t p = 0; p <= 0xFFFF; ++p)
        {
            uint64_t denom = (uint64_t)(p + 1) * (uint64_t)targetFreqHz;
            if (denom == 0)
                continue;
            uint64_t tmp = (uint64_t)timerClock / denom;
            if (tmp == 0)
                continue;
            if (tmp - 1 <= 0xFFFF)
            {
                bestPSC = p;
                bestARR = (uint32_t)(tmp - 1);
                break;
            }
        }
        if (bestARR == 0)
        {
            bestPSC = 0;
            bestARR = (uint32_t)(timerClock / targetFreqHz - 1);
        }

        TIM_TimeBaseInitStructure.TIM_Period = bestARR;
        TIM_TimeBaseInitStructure.TIM_Prescaler = bestPSC;
        TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
        TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
        TIM_TimeBaseInit(TIMx, &TIM_TimeBaseInitStructure);
        TIM_ARRPreloadConfig(TIMx, ENABLE);

        TIM_OCInitTypeDef TIM_OCInitStructure = {0};
        // Configure compare mode but do NOT enable the OC output on the pin.
        // We only need the compare event to generate DMA requests, so keep OutputState disabled.
        TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
        TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Disable;                  // don't drive GPIO
        TIM_OCInitStructure.TIM_Pulse = (TIM_TimeBaseInitStructure.TIM_Period + 1) / 2; // compare point
        TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;

        uint16_t dmaFlag = 0;

        switch (channel)
        {
        case 1:
            TIM_OC1Init(TIMx, &TIM_OCInitStructure);
            dmaFlag = TIM_DMA_CC1;
            break;
        case 2:
            TIM_OC2Init(TIMx, &TIM_OCInitStructure);
            dmaFlag = TIM_DMA_CC2;
            break;
        case 3:
            TIM_OC3Init(TIMx, &TIM_OCInitStructure);
            dmaFlag = TIM_DMA_CC3;
            break;
        case 4:
            TIM_OC4Init(TIMx, &TIM_OCInitStructure);
            dmaFlag = TIM_DMA_CC4;
            break;
        default:
            // invalid channel: do nothing
            return;
        }
        TIM_DMACmd(TIMx, dmaFlag, ENABLE);

        TIM_Cmd(TIMx, ENABLE);
    }

    // Configure DMA channel and prime the buffer for peripheral set/reset writes.
    void configureDMA(DMA_Channel_TypeDef *channel, uint8_t pinName, uint32_t *DMABuffer)
    {

        GPIO_TypeDef *port = get_GPIO_Port(CH_PORT(digitalPinToPinName(pinName)));
        uint32_t pin = CH_GPIO_PIN(digitalPinToPinName(pinName));
        setValue = pin;
        resetValue = pin << 16;

        // Initialize DMASettings here so this helper owns the DMA configuration.
        DMASettings.DMA_PeripheralBaseAddr = (uint32_t)&(port->BSHR);
        DMASettings.DMA_MemoryBaseAddr = (uint32_t)DMABuffer;
        DMASettings.DMA_DIR = DMA_DIR_PeripheralDST;
        DMASettings.DMA_BufferSize = 100;
        DMASettings.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMASettings.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMASettings.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word;
        DMASettings.DMA_MemoryDataSize = DMA_MemoryDataSize_Word;
        DMASettings.DMA_Mode = DMA_Mode_Circular;
        DMASettings.DMA_Priority = DMA_Priority_High;
        DMASettings.DMA_M2M = DMA_M2M_Disable;

        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
        DMA_Init(channel, &DMASettings);
    }    
};

class DMA_SPI_Config
{
public:
    DMA_SPI_Config init(uint8_t *DMABuffer, size_t BufferSize)
    {
        DMAChannel = DMA1_Channel3;
        configureDMA(DMAChannel, DMABuffer, BufferSize);
        // Initialize the SPI peripheral
        SPI.beginTransaction(SPISettings(4800000, MSBFIRST, SPI_MODE0, SPI_TRANSMITONLY));    

        // Set SPI to send DMA request when transmit buffer is empty
        SPI1->CTLR2 |= SPI_CTLR2_TXDMAEN;

        return *this;
    }

    void begin() {
        DMA1->INTFCR = 0xF00; //Clear the flags on channel 3 //Clear the completion flag to indicate a new transfer
        DMA_Init(DMAChannel, &DMASettings);
        DMA_Cmd(DMAChannel, ENABLE);

    }

private:
    DMA_Channel_TypeDef *DMAChannel;
    DMA_InitTypeDef DMASettings;

    // Configure DMA channel and prime the buffer for peripheral set/reset writes.
    void configureDMA(DMA_Channel_TypeDef *channel, uint8_t *DMABuffer, size_t BufferSize)
    {

        // Initialize DMASettings here so this helper owns the DMA configuration.
        DMASettings.DMA_PeripheralBaseAddr = (uint32_t)&(SPI1->DATAR);
        DMASettings.DMA_MemoryBaseAddr = (uint32_t)DMABuffer;
        DMASettings.DMA_DIR = DMA_DIR_PeripheralDST;
        DMASettings.DMA_BufferSize = BufferSize;
        DMASettings.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
        DMASettings.DMA_MemoryInc = DMA_MemoryInc_Enable;
        DMASettings.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
        DMASettings.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
        DMASettings.DMA_Mode = DMA_Mode_Normal;
        DMASettings.DMA_Priority = DMA_Priority_High;
        DMASettings.DMA_M2M = DMA_M2M_Disable;

        RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
        DMA_Init(channel, &DMASettings);
    }

    void configureSPI() {
        SPI_InitTypeDef *SPI_InitStruct = {0};
        SPI_StructInit(SPI_InitStruct);

        SPI_InitStruct->SPI_Direction = SPI_Direction_1Line_Tx;
        SPI_InitStruct->SPI_Mode = SPI_Mode_Master;
        SPI_InitStruct->SPI_DataSize = SPI_DataSize_8b;
        SPI_InitStruct->SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_2;
        /*"SPI_FirstBit_LSB" not support SPI slave mode*/
        SPI_InitStruct->SPI_FirstBit = SPI_FirstBit_MSB;

        SPI_Init(SPI1, SPI_InitStruct);
        SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);

        SPI_Cmd(SPI1, ENABLE);
    }
    
};