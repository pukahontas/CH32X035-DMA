#include <Arduino.h>
#include "dma.cpp"
#include "debug.cpp"
#include <SPI.h>
#include "LEDSPI.h"

//#define SERIAL_ENABLE
#define LED_NUM 10

LED_SPI_CH32 LED_SPI(LED_NUM);

void setup() {

    // Set up serial monitor
    
#ifdef SERIAL_ENABLE
    USBSerial.begin(128);
    //if (!USBSerial.waitForPC(20))
      //  USBSerial.end();
    USBSerial.println("Starting up...");
#endif

    LED_SPI.setLED(0, 255, 0, 0); // Set first LED to red
    LED_SPI.setLED(1, 0, 255, 0); // Set second LED to green
    LED_SPI.setLED(2, 0, 0, 255); // Set third LED to blue
    
}

void loop() {
    delay(100);
    LED_SPI.send();

#ifdef SERIAL_ENABLE
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