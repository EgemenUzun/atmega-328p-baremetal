# avr-blink — ATmega328P bare-metal blink (C++17 + CMake)

A standalone (no Arduino board) ATmega328P running on a 16 MHz crystal,
programmed over a USBASP/USBISP in-system programmer. Firmware is written in
**freestanding C++17**, built with **CMake**, and flashed with **avrdude**.

The program toggles one output pin once per second. Build output is
**162 bytes of flash, 0 bytes of RAM**.

Verified with `avr-g++ 7.3.0` + `avr-libc 2.0.0` (the toolchain in the Ubuntu
24.04 repositories) and CMake 3.28.

---

## 1. Hardware

### 1.1 Bill of materials

| Qty | Part                          | Notes                                  |
|-----|-------------------------------|----------------------------------------|
| 1   | ATmega328P (DIP-28)           | The microcontroller                    |
| 1   | 16 MHz crystal                | HC-49 or similar                       |
| 2   | 22 pF ceramic capacitors      | Crystal load caps                      |
| 1   | 10 kΩ resistor                | RESET pull-up                          |
| 1   | 100 nF ceramic capacitor      | VCC–GND decoupling (recommended)       |
| 1   | USBASP / USBISP programmer    | Provides 5 V and ISP signals           |
| —   | Breadboard + jumper wires     |                                        |
| 1   | LED + ~330 Ω resistor         | Optional output indicator              |

### 1.2 Pinout reference (DIP-28)

```
                  ATmega328P (DIP-28)
                   +-------\_/-------+
   (RESET) PC6 --1 |o              | 28-- PC5
     (RXD) PD0 --2 |               | 27-- PC4
     (TXD) PD1 --3 |               | 26-- PC3
           PD2 --4 |               | 25-- PC2
           PD3 --5 |               | 24-- PC1
     (LED) PD4 --6 |               | 23-- PC0
           VCC --7 |               | 22-- GND
           GND --8 |               | 21-- AREF
   (XTAL1) PB6 --9 |               | 20-- AVCC
   (XTAL2) PB7 -10 |               | 19-- PB5  (SCK, ISP clock)
           PD5 -11 |               | 18-- PB4  (MISO)
           PD6 -12 |               | 17-- PB3  (MOSI)
           PD7 -13 |               | 16-- PB2  (SS)
           PB0 -14 |               | 15-- PB1
                   +---------------+
```

### 1.3 Core circuit (power, clock, reset)

| ATmega328P pin       | Connect to                                   |
|----------------------|----------------------------------------------|
| 7  — VCC             | +5 V                                         |
| 20 — AVCC            | +5 V (recommended; powers ADC and PORTC)     |
| 8  — GND             | Ground                                       |
| 22 — GND             | Ground                                       |
| 1  — RESET           | +5 V **through a 10 kΩ pull-up**             |
| 9  — XTAL1           | One leg of the 16 MHz crystal                |
| 10 — XTAL2           | Other leg of the 16 MHz crystal              |
| (each crystal leg)   | 22 pF capacitor to GND                       |
| VCC ↔ GND            | 100 nF decoupling cap, close to the chip     |

> **RESET is active-LOW.** Pull it **up to VCC** with the 10 kΩ resistor. Tying
> it toward GND holds the chip in reset and it will never run.

### 1.4 ISP wiring (USBASP → ATmega328P)

In-system programming uses SPI. **Connect each signal straight through — same
name to same name. Do NOT cross MOSI/MISO** (crossing is for UART TX/RX, not
SPI).

| USBASP signal | ATmega328P pin | Port pin |
|---------------|----------------|----------|
| VCC           | 7              | —        |
| GND           | 8              | —        |
| MOSI          | 17             | PB3      |
| MISO          | 18             | PB4      |
| SCK           | 19             | PB5      |
| RESET         | 1              | —        |

All six wires are required. Omitting **SCK** (the programming clock) or
**RESET** (lets the programmer enter programming mode) means avrdude cannot talk
to the chip.

### 1.5 Output / LED pin

This project blinks **PD4 (pin 6)**. PD4 is a plain GPIO with no special
function, so the LED stays clear of the ISP SPI lines (MOSI/MISO/SCK on
PB3/PB4/PB5) and in-system programming is never disturbed.

> The Arduino on-board LED is on PB5 (pin 19), but PB5 doubles as the ISP **SCK**
> clock. Putting an LED there works, yet it flickers during programming and can
> load the clock line. Using PD4 avoids that entirely.

Wiring: pin 6 → LED anode, LED cathode → ~330 Ω resistor → GND.

---

## 2. Toolchain / prerequisites

On Ubuntu 24.04 (or newer):

```bash
sudo apt install gcc-avr avr-libc binutils-avr avrdude cmake
```

This installs `avr-gcc`/`avr-g++` 7.3.0 (full C++17 support), `avr-libc`,
`avrdude`, and CMake.

> Ubuntu's `avr-g++` ships only the C standard headers (`<stdint.h>`), **not**
> the C++ wrappers (`<cstdint>`) or libstdc++. That is expected for bare-metal
> AVR. This project uses freestanding C++ only — templates, `constexpr`,
> `static_assert` — with no exceptions, RTTI, or heap allocation.

---

## 3. Building

```bash
cmake -S . -B build -DCMAKE_TOOLCHAIN_FILE=cmake/avr-toolchain.cmake
cmake --build build
```

What happens:

1. The **toolchain file** (`cmake/avr-toolchain.cmake`) switches CMake to the
   `avr-gcc`/`avr-g++` cross-compiler. It sets `CMAKE_SYSTEM_NAME Generic` and
   tells CMake to test the compiler by building a *static library* instead of a
   full executable (a freestanding executable can't link during the sanity
   check without `-mmcu` and the C runtime).
2. `avr-g++` compiles `src/main.cpp` with `-mmcu=atmega328p`,
   `-DF_CPU=16000000UL`, `-std=c++17`, `-Os`, and LTO.
3. The linker produces `build/avr_blink.elf`.
4. A post-build step runs `avr-objcopy` to create **`build/avr_blink.hex`** and
   prints a flash/RAM usage report via `avr-size`.

Expected size report:

```
Program:   162 bytes (0.5% Full)
Data:        0 bytes (0.0% Full)
```

### Key compile flags (set in `CMakeLists.txt`)

| Flag                       | Purpose                                          |
|----------------------------|--------------------------------------------------|
| `-mmcu=atmega328p`         | Target device (compile **and** link)             |
| `-DF_CPU=16000000UL`       | Clock for `_delay_ms()` timing; match the crystal|
| `-std=c++17`               | Language standard                                |
| `-Os`                      | Optimize for size                                |
| `-flto`                    | Link-time optimization (smaller binary)          |
| `-ffunction-sections -fdata-sections` + `--gc-sections` | Drop unused code |
| `-fno-exceptions -fno-rtti`| Remove C++ runtime overhead unused on MCU        |
| `-fno-threadsafe-statics`  | Cheap local-static guards (no locking)           |

---

## 4. Flashing

```bash
cmake --build build --target flash
```

This runs:

```bash
avrdude -c usbasp -p m328p -U flash:w:avr_blink.hex:i
```

A successful run ends with `N bytes of flash verified`. The programmer powers
the chip, so the firmware starts running immediately after flashing.

---

## 5. CMake targets

| Command                                  | Action                              |
|------------------------------------------|-------------------------------------|
| `cmake --build build`                    | Compile → `.elf` + `.hex` + size    |
| `cmake --build build --target flash`     | Upload `.hex` via USBASP            |
| `cmake --build build --target fuses-read`| Read the current fuse bytes (safe)  |

---

## 6. Verifying it works

### 6.1 Communication / signature

```bash
avrdude -c usbasp -p m328p
```

A correct setup reports the device signature `0x1e950f` (ATmega328P). Because
the chip needs a working clock to respond over ISP, a successful read also
**confirms the 16 MHz crystal is oscillating**.

### 6.2 Fuses

This chip reads:

```
lfuse = 0xDE   hfuse = 0xD9   efuse = 0xFF
```

These are already correct for a 16 MHz external crystal with ISP enabled
(`SPIEN` programmed, `RSTDISBL` left unprogrammed, no clock divide).

> **Do not write fuses unless you have a specific reason.** Writing wrong values
> — especially programming `RSTDISBL` or clearing `SPIEN` — disables ISP and can
> brick the chip until you provide a high-voltage programmer or external clock.
> Fuse *writing* is intentionally left commented out in `CMakeLists.txt`.

### 6.3 Measuring the output without an LED

A multimeter is enough — you do not need an LED.

1. A normal multimeter samples a few times per second, so a 1 Hz square wave
   just makes the reading jump. **Slow the blink down first**: change
   `_delay_ms(500)` to `_delay_ms(2000)` in `src/main.cpp`, rebuild, reflash.
2. Set the meter to **DC volts**.
3. Black probe to GND, red probe to **PD4 (pin 6)**.
4. You should see roughly **5 V for ~2 s, then 0 V for ~2 s**, repeating — proof
   the pin is toggling.

If your meter has a **frequency (Hz)** mode, measure the pin directly: ~1 Hz at
`_delay_ms(500)`, ~0.25 Hz at `_delay_ms(2000)`.

---

## 7. Customizing

### Change the target or clock

Edit the top of `CMakeLists.txt`:

```cmake
set(MCU        atmega328p)
set(F_CPU      16000000UL)   # must match the crystal for correct timing
set(PROGRAMMER usbasp)
```

### Change the LED pin

`main.cpp` already defines `PortD` and blinks `PD4`. To use a pin on another
port, add a descriptor for that port and update the `Led` alias. For example, to
move the LED to PB1 (pin 15):

```cpp
namespace gpio {
struct PortB {
    static volatile uint8_t& ddr()  { return DDRB;  }
    static volatile uint8_t& port() { return PORTB; }
    static volatile uint8_t& pin()  { return PINB;  }
};
}

using Led = gpio::Output<gpio::PortB, PB1>;   // pin 15
```

The abstraction is zero-cost: with `-Os`, `Led::init()` compiles to a single
`sbi` instruction and `Led::toggle()` to the standard `in`/`eor`/`out`
read-modify-write — identical to hand-written register code.

---

## 8. VS Code IntelliSense (optional)

If the editor underlines `#include <avr/io.h>` ("cannot open source file"), it
is only the IntelliSense engine missing the AVR paths — the build is unaffected.

The project enables `CMAKE_EXPORT_COMPILE_COMMANDS`, so configuring once writes
`build/compile_commands.json`. Point the Microsoft C/C++ extension at it via
`.vscode/c_cpp_properties.json`:

```json
{
    "version": 4,
    "configurations": [
        {
            "name": "AVR",
            "compilerPath": "/usr/bin/avr-gcc",
            "compilerArgs": ["-mmcu=atmega328p"],
            "compileCommands": "${workspaceFolder}/build/compile_commands.json",
            "cppStandard": "c++17",
            "defines": ["F_CPU=16000000UL", "__AVR_ATmega328P__"],
            "includePath": [
                "/usr/lib/avr/include",
                "/usr/lib/gcc/avr/7.3.0/include",
                "${workspaceFolder}/src"
            ]
        }
    ]
}
```

Then run *C/C++: Select a Configuration → AVR* and reload the window.

---

## 9. Project layout

```
avr-blink/
├── CMakeLists.txt                  # MCU config, C++17 flags, hex/size, targets
├── cmake/
│   └── avr-toolchain.cmake         # avr-gcc/g++ cross-compilation setup
├── src/
│   └── main.cpp                    # blink + zero-cost compile-time GPIO wrapper
├── .vscode/
│   └── c_cpp_properties.json       # IntelliSense config (optional)
└── README.md
```