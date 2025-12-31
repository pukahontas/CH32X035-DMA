#include <Arduino.h>
#include "LEDSPI.h"
#include "FixedPoint.cpp"

//#define SERIAL_ENABLE
#define LED_NUM 21

#ifdef SERIAL_ENABLE
#include "debug.cpp"
#endif

LED_SPI_CH32 LED_SPI(LED_NUM, 3);

void setup()
{
    LED_SPI.start();

#ifdef SERIAL_ENABLE
    // Set up serial monitor
    USBSerial.begin(128);
    // if (!USBSerial.waitForPC(20))
    //   USBSerial.end();
    USBSerial.println("Starting up...");
#endif
}

int t;
uint32_t lastTickTime = micros();
void loop()
{
    // Delay until 10 milliseconds have passed since the last draw call
    while (micros() - lastTickTime < 20000) {}
    lastTickTime = micros();

    for (int i = 0; i < LED_NUM; i++)
    {

        LED_SPI.setLED(i, 
        sinFixed((8 * LED_NUM * t - (512 * i)) / LED_NUM) - 60,
        sinFixed((2 * LED_NUM * t + (300 * i)) / LED_NUM) - 60,
        sinFixed((4 * LED_NUM * t + (800 * i)) / LED_NUM) - 60);
    }
    t++;

#ifdef SERIAL_ENABLE
    USBSerial.println(micros() - lastTickTime);
    return;

    for (int i = 0; i < LED_NUM; i++)
    {
        USBSerial.print(i);
        USBSerial.print(": ");
        USBSerial.print(LED_SPI._LEDColors[i * 3 + 1], BIN);
        USBSerial.println();
    }
    /* for (size_t i = 0; i < LED_SPI._DMABufferSize; i++)
    {
        if (i % BITS_PER_SIGNAL == 0)
        {
            USBSerial.println();
            USBSerial.print(i / BITS_PER_SIGNAL);
            USBSerial.print(": ");
        }
        USBSerial.print(LED_SPI._DMABuffer[i], BIN);
        USBSerial.print(" ");
    }*/
#endif
}