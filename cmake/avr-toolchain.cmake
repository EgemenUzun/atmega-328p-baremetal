# Cross-compilation toolchain for AVR (avr-gcc / avr-g++).
# Pass to CMake with: -DCMAKE_TOOLCHAIN_FILE=cmake/avr-toolchain.cmake

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR avr)

# The compiler-sanity check links a full executable by default, which fails for
# a freestanding target (no -mmcu/crt yet). Building a static lib avoids that.
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

find_program(AVR_CC      avr-gcc     REQUIRED)
find_program(AVR_CXX     avr-g++     REQUIRED)
find_program(AVR_OBJCOPY avr-objcopy REQUIRED)
find_program(AVR_SIZE    avr-size    REQUIRED)
find_program(AVR_OBJDUMP avr-objdump)
find_program(AVRDUDE     avrdude)

set(CMAKE_C_COMPILER   ${AVR_CC})
set(CMAKE_CXX_COMPILER ${AVR_CXX})

# Only search the target sysroot for libs/headers, never for host programs.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
