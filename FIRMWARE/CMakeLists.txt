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
set(sdkVersion 2.3.0)
set(toolchainVersion 15_2_Rel1)
set(picotoolVersion 2.3.0)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
# 独自ボード定義の検索先
list(APPEND PICO_BOARD_HEADER_DIRS
    ${CMAKE_CURRENT_LIST_DIR}/boards
)
set(PICO_BOARD ILF_RP2354B_MSXREADER CACHE STRING "Board type")

# Pico SDK同梱版ではなく、プロジェクトで固定したTinyUSB 0.21.0を使用する
include(FetchContent)
FetchContent_Declare(tinyusb_021
    GIT_REPOSITORY https://github.com/hathach/tinyusb.git
    GIT_TAG dae3f9a366bfcddbf9dcf1b48d7500286a849539
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR cmake-not-used
    PATCH_COMMAND
        ${CMAKE_COMMAND}
        -DTINYUSB_SOURCE_DIR=<SOURCE_DIR>
        -DTINYUSB_PATCH_DIR=${CMAKE_CURRENT_LIST_DIR}/patches/tinyusb
        -P ${CMAKE_CURRENT_LIST_DIR}/cmake/ApplyTinyusbPatches.cmake
)
FetchContent_MakeAvailable(tinyusb_021)
set(PICO_TINYUSB_PATH ${tinyusb_021_SOURCE_DIR} CACHE PATH "TinyUSB 0.21.0 source" FORCE)

# Pull in Raspberry Pi Pico SDK (must be before project)
include(pico_sdk_import.cmake)

project(MSXPLAYer_CR VERSION 2.00 LANGUAGES C CXX ASM)

# Initialise the Raspberry Pi Pico SDK
pico_sdk_init()

# Add executable. Default name is the project name, version 0.1
cmake_minimum_required(VERSION 3.13)

set(MSXPLAYer_CR_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/main.c
        ${CMAKE_CURRENT_LIST_DIR}/commands.c
        ${CMAKE_CURRENT_LIST_DIR}/ports.c
        ${CMAKE_CURRENT_LIST_DIR}/slot_bus_b2.c
        ${CMAKE_CURRENT_LIST_DIR}/slot_bus_rev_f.c
        ${CMAKE_CURRENT_LIST_DIR}/factory_test_dispatch.c
        ${CMAKE_CURRENT_LIST_DIR}/factory_test_b2.c
        ${CMAKE_CURRENT_LIST_DIR}/factory_test_rev_f.c
        ${CMAKE_CURRENT_LIST_DIR}/usb_descriptors.c
)

add_executable(MSXPLAYer_CR)

# Create a directory for generated files
file(MAKE_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/generated)

add_custom_command(
    OUTPUT ${CMAKE_CURRENT_LIST_DIR}/generated/build_timestamp.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_CURRENT_LIST_DIR}/generated
    COMMAND ${CMAKE_COMMAND}
            -DOUTPUT_FILE=${CMAKE_CURRENT_LIST_DIR}/generated/build_timestamp.h
            "-DSOURCE_FILES=${MSXPLAYer_CR_SOURCES}"
            -P ${CMAKE_CURRENT_LIST_DIR}/generate_build_timestamp.cmake
    DEPENDS ${MSXPLAYer_CR_SOURCES}
    COMMENT "Generating build timestamp header from target_sources"
    VERBATIM
)

add_custom_target(build_timestamp_header
    DEPENDS ${CMAKE_CURRENT_LIST_DIR}/generated/build_timestamp.h
)

# Generate the PIO header file from ws2812.pio
pico_generate_pio_header(MSXPLAYer_CR ${CMAKE_CURRENT_LIST_DIR}/ws2812.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(MSXPLAYer_CR ${CMAKE_CURRENT_LIST_DIR}/pwm_low_hiz.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)
pico_generate_pio_header(MSXPLAYer_CR ${CMAKE_CURRENT_LIST_DIR}/pwm_push_pull.pio OUTPUT_DIR ${CMAKE_CURRENT_LIST_DIR}/generated)

target_sources(MSXPLAYer_CR PUBLIC
        ${MSXPLAYer_CR_SOURCES}
)

add_dependencies(MSXPLAYer_CR build_timestamp_header)

# Make sure TinyUSB can find tusb_config.h
target_include_directories(MSXPLAYer_CR PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/WS2812/include
        ${CMAKE_CURRENT_LIST_DIR}/generated
        )

# In addition to pico_stdlib required for common PicoSDK functionality, add dependency on tinyusb_device
# for TinyUSB device support and tinyusb_board for the additional board support library used by the example
target_link_libraries(MSXPLAYer_CR 
    PUBLIC pico_stdlib 
    pico_unique_id 
    tinyusb_device 
    tinyusb_board 
    hardware_pwm
    hardware_clocks
    hardware_pio
    hardware_gpio 
    pico_multicore)

# pico_enable_stdio_usb(MSXPLAYer_CR 1)
pico_enable_stdio_uart(MSXPLAYer_CR 1)
pico_add_extra_outputs(MSXPLAYer_CR)

# add url via pico_set_program_url
target_link_options(${PROJECT_NAME} PRIVATE -Wl,-Map=${PROJECT_NAME}.map)
target_link_options(${PROJECT_NAME} PRIVATE -Wl,--print-memory-usage)
