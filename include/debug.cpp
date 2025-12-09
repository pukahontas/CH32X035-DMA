#pragma once
#include <CH32X035_USBSerial.h>
using namespace wch::usbcdc;

void printAddr (volatile uint32_t* addr, String name = "", int base = 16) {
    USBSerial.print(name);
    USBSerial.print(" @ 0x");
    USBSerial.println((uint32_t)addr, HEX);

    if (base == 2) {
        USBSerial.print("0b");
        for (int8_t i = 31; i >= 0; i--) {
            USBSerial.print(((*addr >> i) & 0x1) ? '1' : '0');
            if (i % 4 == 0) USBSerial.print(' ');
        }
    } else if (base == 16) {    
        USBSerial.print("0x");
        for (uint32_t i = 8; i; i--) {
            USBSerial.print(((*addr >> ((i - 1) * 4)) & 0xF), HEX);
        }
        USBSerial.println();
    } else
        USBSerial.println(*addr, base);

    USBSerial.println();
}

void printAddr (volatile uint16_t* addr, String name = "", int base = 16) {
    printAddr((volatile uint32_t*)addr, name, base);
}