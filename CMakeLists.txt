cmake_minimum_required(VERSION 3.20)
project(avr_blink LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Target configuration -------------------------------------------------
set(MCU        atmega328p)
set(F_CPU      16000000UL)     # 16 MHz crystal
set(PROGRAMMER usbasp)

# ---- Language standard ----------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)  # -> -std=c++17 (not gnu++17)

# ---- Compile flags --------------------------------------------------------
add_compile_options(
    -mmcu=${MCU}
    -DF_CPU=${F_CPU}
    -Os
    -flto
    -ffunction-sections -fdata-sections
    -funsigned-char -funsigned-bitfields
    -Wall -Wextra
    # C++ on a tiny MCU: no exceptions, no RTTI, cheap local-static guards.
    $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
    $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
)

# ---- Link flags -----------------------------------------------------------
add_link_options(
    -mmcu=${MCU}
    -flto
    -Wl,--gc-sections
    -Wl,-Map=${PROJECT_NAME}.map
)

# ---- Build the firmware ---------------------------------------------------
add_executable(${PROJECT_NAME}.elf src/main.cpp)
target_include_directories(${PROJECT_NAME}.elf PRIVATE include)

# .elf -> .hex  +  flash-usage report, every build.
add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${AVR_OBJCOPY} -O ihex -R .eeprom
            $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${AVR_OBJCOPY} -O ihex -j .eeprom
            --change-section-lma .eeprom=0 --no-change-warnings
            $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.eep || true
    COMMAND ${AVR_SIZE} --format=avr --mcu=${MCU}
            $<TARGET_FILE:${PROJECT_NAME}.elf>
    COMMENT "Generating ${PROJECT_NAME}.hex and size report"
    VERBATIM
)

# ---- Convenience targets --------------------------------------------------
# Upload firmware:   cmake --build build --target flash
add_custom_target(flash
    COMMAND ${AVRDUDE} -c ${PROGRAMMER} -p ${MCU}
            -U flash:w:${PROJECT_NAME}.hex:i
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing ${PROJECT_NAME}.hex via ${PROGRAMMER}"
    VERBATIM
)

# Read fuses (safe):  cmake --build build --target fuses-read
add_custom_target(fuses-read
    COMMAND ${AVRDUDE} -c ${PROGRAMMER} -p ${MCU}
            -U lfuse:r:-:h -U hfuse:r:-:h -U efuse:r:-:h
    COMMENT "Reading current fuse bytes"
    VERBATIM
)

# WARNING: writing fuses can brick ISP access if values are wrong.
# Your chip already reads L:DE H:D9 E:FF (correct for a 16 MHz crystal),
# so you normally do NOT need this. Uncomment only if you know why.
# add_custom_target(fuses-write
#     COMMAND ${AVRDUDE} -c ${PROGRAMMER} -p ${MCU}
#             -U lfuse:w:0xDE:m -U hfuse:w:0xD9:m -U efuse:w:0xFF:m
#     COMMENT "Writing fuses (DANGEROUS)"
#     VERBATIM
# )