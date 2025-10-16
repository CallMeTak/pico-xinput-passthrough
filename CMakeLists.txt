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

set(PROJECT_NAME passthrough)
project(${PROJECT_NAME} C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()


add_executable(${PROJECT_NAME})
target_sources(${PROJECT_NAME} PRIVATE
    src/passthrough.c
    src/device_callbacks.c
    src/host_callbacks.c
    src/usb_descriptors.c
    src/stdio_usb.c

    # Required for PICO-PIO-USB to work
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/dcd_pio_usb.c
    ${PICO_TINYUSB_PATH}/src/portable/raspberrypi/pio_usb/hcd_pio_usb.c
    )
# Enables tinyusb debug output
target_compile_definitions(${PROJECT_NAME} PUBLIC LOG=1)

target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
pico_set_program_name(${PROJECT_NAME} "${PROJECT_NAME}")
pico_set_program_version(${PROJECT_NAME} "0.1")

#========================================================
# External Libraries
#========================================================

# For USB support via PIO
add_subdirectory(lib/Pico-PIO-USB)

# Required since tusb_xinput and tinyusb-xinput-device links against "tinyusb"
add_library(tinyusb INTERFACE)
target_link_libraries(tinyusb INTERFACE tinyusb_host tinyusb_board)

# XInput host library
add_subdirectory(lib/tusb_xinput)

# XInput device library
add_subdirectory(lib/tinyusb-xinput-device)
# Add the standard library to the build
target_link_libraries(${PROJECT_NAME}
        pico_stdlib
        pico_stdio
        )

# Add the standard include files to the build
target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}/src/include

        # Required for stdio over USB
        ${PICO_SDK_PATH}/src/rp2_common/pico_stdio_usb/include
        
)

# Add any user requested libraries
target_link_libraries(${PROJECT_NAME}
        hardware_timer
        tinyusb_device
        tinyusb_host
        tinyusb_board
        pico_pio_usb
        xinput_host
        tud_xinput
        )

# create map/bin/hex/uf2 file etc.
pico_add_extra_outputs(${PROJECT_NAME})