#include <Arduino.h>
#include "LEDSPI.h"

//#define SERIAL_ENABLE
#define LED_NUM 21

#ifdef SERIAL_ENABLE
#include "debug.cpp"
#endif

LED_SPI_CH32 LED_SPI(LED_NUM);

uint32_t irq = 0b10101010;
uint8_t r, g, b;

typedef struct
{
    uint32_t RESERVED1;
    uint32_t RESERVED2;
    uint32_t NMI;
    uint32_t HardFault;
    uint32_t RESERVED3;
    uint32_t Ecall_M;
    uint32_t RESERVED4[2];
    uint32_t EcallU;
    uint32_t BreakPoint;
    uint32_t RESERVED5[2];
    uint32_t SYSTICK;
    uint32_t RESERVED6;
    uint32_t SW;
    uint32_t RESERVED7;
    uint32_t WWDG_;
    uint32_t PVD;
    uint32_t FLASH_;
    uint32_t RESERVED;
    uint32_t EXT17_0;
    uint32_t AWU_;
    uint32_t DMA1_CH1;
    uint32_t DMA1_CH2;
    uint32_t DMA1_CH3;
    uint32_t DMA1_CH4;
    uint32_t DMA1_CH5;
    uint32_t DMA1_CH6;
    uint32_t DMA1_CH7;
    uint32_t ADC1_;
    uint32_t I2C1_EV;
    uint32_t I2C1_ER;
    uint32_t USART1_;
    uint32_t SPI1_;
    uint32_t TIM1BRK;
    uint32_t TIM1UP;
    uint32_t TIM1TRG;
    uint32_t TIM1CC;
} PFIC_VECTOR_Type;

#define PFIC_VECTOR ((PFIC_VECTOR_Type *)0x4)

extern "C"
{
    void DMA1_Channel3_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
    void DMA1_Channel3_IRQHandler(void)
    {
        irq++;
    }
}

uint32_t count = 0;
float cnt = 0;
void processTick()
{
    for (int i = 0; i < LED_NUM; i++)
    {
        LED_SPI.setLED(i, (uint8_t)round((sin(2. * PI * (8. * r / 256.0 - .5 * i / LED_NUM)) + .8) * 1.0),
                          (uint8_t)round((sin(2. * PI * (2. * g / 256.0 + 0.3 * i / LED_NUM)) + .8) * 1.0),
                          (uint8_t)round((sin(2. * PI * (4. * b / 256.0 + .8 * i / LED_NUM)) + .8) * 1.0));
    }
    r++;
    g++;
    b++;
}

void setup()
{
    // Configure TIM3 to generate an interrupt every 10 ms (100 Hz)
    HardwareTimer *timer = new HardwareTimer(TIM1);

    // Set overflow to 100 Hz (1000 ms / 10 ms = 100 Hz)
    timer->setOverflow(100, HERTZ_FORMAT);
    
    // Attach the interrupt callback for update events
    timer->attachInterrupt(processTick);

    // Start the timer
    timer->resume();

    // Set up interrupt on DMA transfer
    // NVIC_EnableIRQ(DMA1_Channel3_IRQn);
    // PFIC_VECTOR->DMA1_CH3 = 0x534;

    //SetVTFIRQ((uint32_t)&DMA1_Channel3_IRQHandler, DMA1_Channel3_IRQn, 0, ENABLE);
    NVIC_EnableIRQ(DMA1_Channel3_IRQn);

    // Set up serial monitor

#ifdef SERIAL_ENABLE
    USBSerial.begin(128);
    // if (!USBSerial.waitForPC(20))
    //   USBSerial.end();
    USBSerial.println("Starting up...");
#endif
}

void loop()
{

    while (LED_SPI.busy());
    LED_SPI.sendColors();

    while (LED_SPI.busy());
    LED_SPI.sendWait();

    // Enable DMA1 Channel 3 interrupt in NVIC
    //if ((r >> 3) == 10)
        //NVIC_EnableIRQ(DMA1_Channel3_IRQn);

#ifdef SERIAL_ENABLE
    // Print out DMA buffer
    /*for (size_t i = 0; i < LED_SPI._DMABufferSize; i++)
    {
        if (i % BITS_PER_SIGNAL == 0)
        {
            USBSerial.println();
            USBSerial.print(i / BITS_PER_SIGNAL);
            USBSerial.print(": ");
        }
        USBSerial.print(LED_SPI._DMABuffer[i], BIN);
        USBSerial.print(" ");
    }

    // Print out DMA control register information
    /*
    printAddr(&SPI1->CTLR1, "CTLR1", BIN);
    printAddr(&SPI1->CTLR2, "CTLR2", BIN);
    printAddr(&SPI1->STATR, "STATR", BIN);
    // USBSerial.println(status, BIN);
    printAddr(&DMA1->INTFR, "DMA1->INTFR", BIN);
    printAddr(&DMA1_Channel3->MADDR, "Memory Addr", HEX);
    USBSerial.println(LED_SPI._DMABuffer[0], HEX);
    printAddr(&DMA1_Channel3->CNTR, "Current Count", DEC);
    printAddr(&DMA1_Channel3->CFGR, "DMA1 CFGR", BIN);*/

    // Print out PFIC (Programmable Fast Interrupt Controller)
    println();
    printAddr(&PFIC->ISR[0], "ISR1", BIN);
    printAddr(&PFIC->ISR[1], "ISR2", BIN);
    printAddr(&PFIC->ISR[2], "ISR3", BIN);
    printAddr(&PFIC->ISR[3], "ISR4", BIN);

    // Print out VTF (Vector table free interrupt handling)
    println();
    printAddr((volatile uint32_t *)&PFIC->VTFIDR[0], "VTF Interrupt 1", HEX);
    printAddr(&PFIC->VTFADDR[0], "VTF0 Addr", BIN);
    printAddr(&PFIC->VTFADDR[1], "VTF1 Addr", BIN);
    printAddr(&PFIC->VTFADDR[2], "VTF2 Addr", BIN);
    printAddr(&PFIC->VTFADDR[3], "VTF3 Addr", BIN);

    println();
    char str[] = "Interrupt Vector #00";
    for (char i = 0; i <= 54; i++)
    {
        str[19] = '0' + i % 10;
        str[18] = '0' + (i / 10) % 10;
        printAddr((volatile uint32_t *)(i * 4 + 0x08000004), str, HEX, true);
    }

    println();
    printFunctionAddr(&DMA1_Channel3_IRQHandler, "DMA1CH3 IRQ Handler");
    // printFunctionAddr(&TIM1_UP_IRQHandler, "TIM1UP IRQ Handler");

    USBSerial.print("IRQ: ");
    USBSerial.print(irq);
    println();

#endif
}