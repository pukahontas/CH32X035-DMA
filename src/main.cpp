#include <Arduino.h>
#include "dma.cpp"
#include "debug.cpp"
#include <SPI.h>
#include "LEDSPI.h"

//#define SERIAL_ENABLE
#define LED_NUM 21

LED_SPI_CH32 LED_SPI(LED_NUM);

void setup() {

    // Set up serial monitor
    
#ifdef SERIAL_ENABLE
    USBSerial.begin(128);
    //if (!USBSerial.waitForPC(20))
      //  USBSerial.end();
    USBSerial.println("Starting up...");
#endif
    
}

float r, g, b;
void loop() {
    delay(1);
    for (int i = 0; i < 20; i++) {
        LED_SPI.setLED(i, (uint8_t)round(sin(2. * PI * r * .001 * sin(0.0 + i)) * 25.0),
                          (uint8_t)round(sin(2. * PI * g * .002 * sin(1.0 + i)) * 25.0),
                          (uint8_t)round(sin(2. * PI * b * .003 * sin(2.0 + i)) * 25.0));
    }
    r++;
    g++;
    b++;

    LED_SPI.send();

#ifdef SERIAL_ENABLE
    for (size_t i = 0; i < LED_SPI._DMABufferSize; i++) {
        if (i % BITS_PER_SIGNAL == 0) {
            USBSerial.println();
            USBSerial.print(i / BITS_PER_SIGNAL);
            USBSerial.print(": ");
        }
        USBSerial.print(LED_SPI._DMABuffer[i],  BIN);
        USBSerial.print(" ");
    }


    return;

    printAddr(&SPI1->CTLR1, "CTLR1", BIN);
    printAddr(&SPI1->CTLR2, "CTLR2", BIN);
    printAddr(&SPI1->STATR, "STATR", BIN);
    //USBSerial.println(status, BIN);
    printAddr(&DMA1->INTFR, "DMA1->INTFR", BIN);
    printAddr(&DMA1_Channel3->MADDR, "Memory Addr", HEX);
    USBSerial.println(LED_SPI._DMABuffer[0],  HEX);
    printAddr(&DMA1_Channel3->CNTR, "Current Count", DEC);
    printAddr(&DMA1_Channel3->CFGR, "DMA1 CFGR", BIN);
#endif

}