#pragma once
//
// gpio.hpp -- minimal zero-overhead GPIO library for the ATmega328P
//             (and pin/register-compatible AVRs).
//
// Every method compiles to a single sbi/cbi/in/out under -Os: the templates
// disappear and leave hand-written register code. No allocation, no virtuals,
// no runtime cost.
//
// IMPORTANT: the ATmega328P has only PORTB, PORTC and PORTD. There is NO PORTA
// (that exists on chips such as the ATmega16/32/1284). PORTC is 7 bits wide
// (PC0..PC6; PC6 is normally the RESET pin), so do not use PC7.

#include <avr/io.h>
#include <stdint.h>

namespace gpio {

// ---------------------------------------------------------------------------
// Port descriptors: each bundles a port's three SFRs as inlinable accessors.
// Using the avr-libc macros (not raw addresses) keeps them always-correct.
// ---------------------------------------------------------------------------
struct PortB {
    static volatile uint8_t& ddr()  { return DDRB;  }
    static volatile uint8_t& port() { return PORTB; }
    static volatile uint8_t& pin()  { return PINB;  }
};

struct PortC {
    static volatile uint8_t& ddr()  { return DDRC;  }
    static volatile uint8_t& port() { return PORTC; }
    static volatile uint8_t& pin()  { return PINC;  }
};

struct PortD {
    static volatile uint8_t& ddr()  { return DDRD;  }
    static volatile uint8_t& port() { return PORTD; }
    static volatile uint8_t& pin()  { return PIND;  }
};

// Internal pull-up selection for inputs.
enum class Pull : uint8_t { None, Up };

// ---------------------------------------------------------------------------
// Output pin: actively drives the line high or low.
// ---------------------------------------------------------------------------
template <typename Port, uint8_t Bit>
class Output {
    static_assert(Bit < 8, "GPIO bit must be 0..7");
    static constexpr uint8_t mask = static_cast<uint8_t>(1u << Bit);

public:
    static void init()       { Port::ddr() |= mask; }                       // -> output
    static void high()       { Port::port() |= mask; }
    static void low()        { Port::port() &= static_cast<uint8_t>(~mask); }
    static void toggle()     { Port::port() ^= mask; }
    static void set(bool on) { on ? high() : low(); }
};

// ---------------------------------------------------------------------------
// Input pin: reads the line, with optional internal pull-up.
// With Pull::Up an idle line reads HIGH and a switch to GND reads LOW
// (the standard active-low button wiring -- no external resistor needed).
// ---------------------------------------------------------------------------
template <typename Port, uint8_t Bit>
class Input {
    static_assert(Bit < 8, "GPIO bit must be 0..7");
    static constexpr uint8_t mask = static_cast<uint8_t>(1u << Bit);

public:
    static void init(Pull pull = Pull::None) {
        Port::ddr() &= static_cast<uint8_t>(~mask);     // -> input
        if (pull == Pull::Up) Port::port() |= mask;     // enable internal pull-up
        else                  Port::port() &= static_cast<uint8_t>(~mask);
    }
    static bool read()   { return (Port::pin() & mask) != 0; }   // raw pin level
    static bool isHigh() { return read(); }
    static bool isLow()  { return !read(); }
};

// ---------------------------------------------------------------------------
// Open-drain pin: drives LOW actively, or "releases" to high-impedance and
// lets an EXTERNAL pull-up bring the line high. Multiple devices can share the
// line safely (wired-AND). This is exactly the electrical model used by I2C
// SDA/SCL, so the same class will be reused for the bit-banged I2C driver.
//
// The PORT bit is kept at 0, so the pin is always either driven-low or hi-Z.
// An external pull-up resistor is REQUIRED (the internal pull-up cannot be used
// here, because the PORT bit doubles as the drive value).
// ---------------------------------------------------------------------------
template <typename Port, uint8_t Bit>
class OpenDrain {
    static_assert(Bit < 8, "GPIO bit must be 0..7");
    static constexpr uint8_t mask = static_cast<uint8_t>(1u << Bit);

public:
    static void init() {
        Port::port() &= static_cast<uint8_t>(~mask);    // drive value = 0, no pull-up
        Port::ddr()  &= static_cast<uint8_t>(~mask);    // start released (hi-Z)
    }
    static void low()     { Port::ddr() |= mask; }                          // pull to 0
    static void release() { Port::ddr() &= static_cast<uint8_t>(~mask); }   // hi-Z -> pulled high
    static bool read()    { return (Port::pin() & mask) != 0; }
};

}  // namespace gpio
