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
set(picotoolVersion 2.2.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(passthrough C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()



add_executable(passthrough src/passthrough.c src/usb_descriptors.c )
target_sources(passthrough PRIVATE
    src/passthrough.c
    src/device_callbacks.c
    src/host_callbacks.c
    src/usb_descriptors.c
    src/drivers/stdio_usb.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
    )
target_compile_definitions(passthrough PRIVATE PIO_USB_USE_TINYUSB)
target_compile_options(passthrough PRIVATE -Wall -Wextra)
pico_set_program_name(passthrough "passthrough")
pico_set_program_version(passthrough "0.1")
add_subdirectory(lib/Pico-PIO-USB)
# Add the standard library to the build
target_link_libraries(passthrough
        pico_pio_usb
        tinyusb_device
        tinyusb_host
        tinyusb_board
        pico_stdlib
        pico_stdio
        )

# Add the standard include files to the build
target_include_directories(passthrough PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/include
        ${PICO_SDK_PATH}/src/rp2_common/pico_stdio_usb/include
        
)

# Add any user requested libraries
target_link_libraries(passthrough 
        hardware_timer
        )

pico_add_extra_outputs(passthrough)
add_definitions(-DPICO_STDIO_USB_CONNECTION_WITHOUT_DTR=1)