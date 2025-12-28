#include <Arduino.h>
#include "debug.cpp"
#include "LEDSPI.h"

//#define SERIAL_ENABLE
#define LED_NUM 21

LED_SPI_CH32 LED_SPI(LED_NUM, 2);

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

uint8_t r, g, b;
void loop()
{ 
    for (int i = 0; i < LED_NUM; i++)
    {
        LED_SPI.setLED(i, (sin(2. * PI * (8. * r / 256.0 - .5 * i / LED_NUM)) + .8),
                          (sin(2. * PI * (2. * g / 256.0 + 0.3 * i / LED_NUM)) + .8),
                          (sin(2. * PI * (4. * b / 256.0 + .8 * i / LED_NUM)) + .8));
        //LED_SPI.setLED(i, (float)i/LED_NUM /64, 0.0, 0.0);
    }
    r++;
    g++;
    b++;
    delay(10);

#ifdef SERIAL_ENABLE
        for (int i = 0; i < LED_NUM; i++)
    {
        USBSerial.print(i); USBSerial.print(": ");
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