#include <Arduino.h>
#include "dma.cpp"
#include "debug.cpp"
#include <SPI.h>
#include "LEDSPI.h"

const uint8_t ledPin = PA7;

//#define SERIAL_ENABLE
#define LED_NUM 10
#define BITS_PER_SIGNAL 3
#define SIGNAL_LOW 0b100
#define SIGNAL_HIGH 0b110
#define BUFFER_SIZE (10 * 3 * BITS_PER_SIGNAL) // 10 LEDs, 3 color channels, 8 bits per channel
//uint8_t _DMABuffer[BUFFER_SIZE] = {0};

//DMA_TIM_Config dmaTimConfig;
//DMA_SPI_Config dmaSpiConfig;

LED_SPI_CH32 LED_SPI(LED_NUM);

void setLED(uint16_t index, uint8_t r, uint8_t g, uint8_t b)
{
    if (index >= LED_NUM)
        return;

    size_t dmaIndex = index * 3 * BITS_PER_SIGNAL;

    for (char i = 0; i < 3; i++)
    {
        uint8_t colorChannel = (i == 0) ? g : (i == 1) ? r
                                                       : b; // WS2812 uses GRB order

        // Lookup the WS2812 bit pattern for the high and low nibbles and combine them
        uint32_t ws2812BitPattern = WS2812BitPattern((colorChannel >> 4) & 0x0F) << 12 |
                                    WS2812BitPattern(colorChannel & 0x0F);

        // Assign the 24 bits (3 bytes) of the WS2812 bit pattern for each color channel to the DMA buffer
        LED_SPI._DMABuffer[dmaIndex] = (ws2812BitPattern >> 16) & 0xFF;
        LED_SPI._DMABuffer[dmaIndex++] = (ws2812BitPattern >> 8) & 0xFF;
        LED_SPI._DMABuffer[dmaIndex++] = ws2812BitPattern & 0xFF;
    }
}

void setup() {

    // Set up serial monitor
    
#ifdef SERIAL_ENABLE
    USBSerial.begin(128);
    //if (!USBSerial.waitForPC(20))
      //  USBSerial.end();
    USBSerial.println("Starting up...");
#endif

    pinMode(ledPin, OUTPUT);

    //dmaSpiConfig.init((uint8_t*)_DMABuffer, BUFFER_SIZE);
    //dmaSpiConfig.begin();

    LED_SPI.init(); // Initialize for 300 LEDs
    //DMA1_Channel3->MADDR = (uint32_t)(LED_SPI.getBuffer());
    //LED_SPI.begin();

    setLED(0, 255, 0, 0); // Set first LED to red
    setLED(1, 0, 255, 0); // Set second LED to green
    setLED(2, 0, 0, 255); // Set third LED to blue
    
}

void loop() {
    delay(100);
    LED_SPI.begin();

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