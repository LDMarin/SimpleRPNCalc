# Generated Cmake Pico project file

cmake_minimum_required(VERSION 3.13)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Initialise pico_sdk from installed location
# (note this can come from environment, CMake cache etc)

# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(SimpleRPNCalc C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable - Updated to compile all components together
add_executable(SimpleRPNCalc 
    main.c
    rpn_stack.c
    max7219.c
    keypad.c
    rpn_nvram.c
)

pico_set_program_name(SimpleRPNCalc "SimpleRPNCalc")
pico_set_program_version(SimpleRPNCalc "0.1")

# Modify the below lines to enable/disable output over UART/USB
pico_enable_stdio_uart(SimpleRPNCalc 0)
pico_enable_stdio_usb(SimpleRPNCalc 1) # Turned USB stdio ON for easy debugging if needed

pico_set_printf_implementation(SimpleRPNCalc compiler)

# Target link libraries block (Combines standard libs, hardware SPI, and the C math engine library)
target_link_libraries(SimpleRPNCalc
    pico_stdlib
    hardware_spi
    hardware_i2c
    m
)

# Add the standard include files to the build
target_include_directories(SimpleRPNCalc PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
)

pico_add_extra_outputs(SimpleRPNCalc)

