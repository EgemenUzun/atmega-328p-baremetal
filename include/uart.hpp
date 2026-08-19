#pragma once
//
// uart.hpp -- blocking USART0 driver for the ATmega328P, 8N1.
//
// The baud divisor (UBRR) and whether to use double-speed mode (U2X) are
// computed at COMPILE TIME from F_CPU and the Baud template argument: the
// constructor below picks whichever of the two modes has the smaller rounding
// error, exactly matching the values in the datasheet's baud tables. No
// floating point and no runtime division end up in the firmware.
//
// Wiring (needs a USB-to-serial adapter -- the USBASP cannot do serial):
//   adapter TX  -> ATmega RXD (PD0, pin 2)
//   adapter RX  -> ATmega TXD (PD1, pin 3)
//   adapter GND -> ATmega GND
// Note: TX<->RX IS crossed here. That is correct for UART (unlike SPI/ISP,
// where same-name pins connect straight).

#include <avr/io.h>
#include <stdint.h>

namespace uart {

struct BaudSetting {
    uint16_t ubrr;
    bool     u2x;
};

// Compile-time UBRR + U2X selection (rounds to nearest, picks lower error).
constexpr BaudSetting calc_baud(uint32_t f_cpu, uint32_t baud) {
    uint32_t ubrr_n = (f_cpu + 8UL * baud) / (16UL * baud) - 1;   // normal mode
    uint32_t real_n = f_cpu / (16UL * (ubrr_n + 1));
    uint32_t ubrr_d = (f_cpu + 4UL * baud) / (8UL * baud) - 1;    // double speed
    uint32_t real_d = f_cpu / (8UL * (ubrr_d + 1));

    int32_t err_n = static_cast<int32_t>(real_n) - static_cast<int32_t>(baud);
    if (err_n < 0) err_n = -err_n;
    int32_t err_d = static_cast<int32_t>(real_d) - static_cast<int32_t>(baud);
    if (err_d < 0) err_d = -err_d;

    if (err_d < err_n) return { static_cast<uint16_t>(ubrr_d), true  };
    return                     { static_cast<uint16_t>(ubrr_n), false };
}

// ATmega328P has a single USART0, so all members are static.
template <uint32_t Baud = 9600>
class Usart0 {
    static constexpr BaudSetting cfg = calc_baud(F_CPU, Baud);

public:
    static void init() {
        UBRR0H = static_cast<uint8_t>(cfg.ubrr >> 8);
        UBRR0L = static_cast<uint8_t>(cfg.ubrr);
        UCSR0A = cfg.u2x ? static_cast<uint8_t>(1 << U2X0) : 0;
        UCSR0B = (1 << RXEN0) | (1 << TXEN0);            // enable RX and TX
        UCSR0C = (1 << UCSZ01) | (1 << UCSZ00);          // 8 data bits, no parity, 1 stop
    }

    // ---- transmit ----
    static void write(uint8_t byte) {
        while (!(UCSR0A & (1 << UDRE0))) {}              // wait for empty buffer
        UDR0 = byte;
    }

    static void print(const char* s) {
        while (*s) write(static_cast<uint8_t>(*s++));
    }

    static void println(const char* s) {
        print(s);
        write('\r');
        write('\n');
    }

    // Unsigned decimal, no printf needed (handy for ADC values, counters).
    static void printDec(uint16_t v) {
        char buf[5];                                     // max "65535"
        uint8_t i = 0;
        do { buf[i++] = static_cast<char>('0' + v % 10); v /= 10; } while (v);
        while (i) write(static_cast<uint8_t>(buf[--i]));
    }

    static void printHex(uint8_t v) {
        const char* hex = "0123456789ABCDEF";
        write(static_cast<uint8_t>(hex[v >> 4]));
        write(static_cast<uint8_t>(hex[v & 0x0F]));
    }

    // ---- receive ----
    static bool available() { return (UCSR0A & (1 << RXC0)) != 0; }

    static uint8_t read() {                              // blocks until a byte arrives
        while (!available()) {}
        return UDR0;
    }
};

}  // namespace uart
