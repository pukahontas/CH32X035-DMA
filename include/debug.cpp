#pragma once
#include <CH32X035_USBSerial.h>
using namespace wch::usbcdc;

void print(String str) {
    USBSerial.print(str);
}

void println(String str) {
    USBSerial.println(str);
}

void println() {
    println("");
}

void printPointer (volatile uint32_t* addr, int base = 16) {
    if (base == 2) {
        USBSerial.print("0b");
        for (int8_t i = 31; i >= 0; i--) {
            USBSerial.print(((*addr >> i) & 0x1) ? '1' : '0');
            if (i % 4 == 0) USBSerial.print(' ');
        }
        USBSerial.println();
    } else if (base == 16) {    
        USBSerial.print("0x");
        for (uint32_t i = 8; i; i--) {
            USBSerial.print(((*addr >> ((i - 1) * 4)) & 0xF), HEX);
        }

    } else
        USBSerial.print(*addr, base);
}

void printAddr (volatile uint32_t* addr, String name = "", int base = 16, bool singleLine = false) {
    USBSerial.print(name);
    USBSerial.printf(" @ 0x%08X : ", (uint32_t)addr);

    if (!singleLine)
        println();

    printPointer(addr, base);
    println();
}

void printAddr (volatile uint16_t* addr, String name = "", int base = 16) {
    printAddr((volatile uint32_t*)addr, name, base);
}

void printFunctionAddr(std::function<void()> fn, String name = "") {
    printAddr((volatile uint32_t*)&fn, name, HEX);
}