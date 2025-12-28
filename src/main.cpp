#include <Arduino.h>
#include "debug.cpp"
#include "LEDSPI.h"

//#define SERIAL_ENABLE
#define LED_NUM 21

LED_SPI_CH32 LED_SPI(LED_NUM);

void processTick()
{
    static uint8_t r, g, b;
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
    HardwareTimer* timer = new HardwareTimer(TIM1);
    
    // Set overflow to 100 Hz (1000 ms / 10 ms = 100 Hz)
    timer->setOverflow(100, HERTZ_FORMAT);
    
    // Attach the interrupt callback for update events
    timer->attachInterrupt(processTick);
    
    // Start the timer
    timer->resume();

    LED_SPI.start();
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

    /*while (LED_SPI.busy());
    LED_SPI.sendColors();

    while (LED_SPI.busy());
    LED_SPI.sendWait();*/

    // Enable DMA1 Channel 3 interrupt in NVIC
    // if (r == 2550)

#ifdef SERIAL_ENABLE
    for (size_t i = 0; i < LED_SPI._DMABufferSize; i++)
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

    printAddr(&SPI1->CTLR1, "CTLR1", BIN);
    printAddr(&SPI1->CTLR2, "CTLR2", BIN);
    printAddr(&SPI1->STATR, "STATR", BIN);
    // USBSerial.println(status, BIN);
    printAddr(&DMA1->INTFR, "DMA1->INTFR", BIN);
    printAddr(&DMA1_Channel3->MADDR, "Memory Addr", HEX);
    printAddr((uint32_t*)(DMA1_Channel3->MADDR), "Memory Addr", HEX);
    USBSerial.println(LED_SPI._DMABuffer[0], HEX);
    printAddr(&DMA1_Channel3->CNTR, "Current Count", DEC);
    printAddr(&DMA1_Channel3->CFGR, "DMA1 CFGR", BIN);
#endif
}