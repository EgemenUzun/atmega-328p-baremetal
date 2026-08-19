// Bare-metal UART demo for ATmega328P @ 16 MHz, C++17.
//
//   LED on PD4 (pin 6).
//   USART0 at 9600 8N1 on PD0/PD1 (pins 2/3) via a USB-to-serial adapter.
//
// On boot it prints a greeting. Then it echoes every character you type in the
// serial terminal and toggles the LED on each received byte.

#include "gpio.hpp"
#include "uart.hpp"
#include <util/delay.h>

using Led    = gpio::Output<gpio::PortD, PD4>;
using Serial = uart::Usart0<9600>;

int main() {
    Led::init();
    Serial::init();

    Serial::println("ATmega328P UART ready");

    for (;;) {
        if (Serial::available()) {
            uint8_t c = Serial::read();
            Serial::write(c);        // echo back          // blink on each byte
        }
        Led::toggle();  
        _delay_ms(5000);
    }
}