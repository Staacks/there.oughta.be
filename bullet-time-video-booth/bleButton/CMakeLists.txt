#Massive thanks to https://forums.raspberrypi.com/viewtopic.php?t=347395

cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

set(PICO_PLATFORM rp2040)
set(PICO_BOARD "pico_w")

include(pico_sdk_import.cmake)

project(blebutton C CXX ASM)

pico_sdk_init()


set(CMAKE_BUILD_TYPE Release)


add_executable(blebutton
picow_bt_example_background.c
blebutton.c
)
target_link_libraries(blebutton
pico_stdlib
pico_btstack_ble
pico_btstack_cyw43
pico_cyw43_arch_none
)
pico_enable_stdio_usb(blebutton 1)
pico_enable_stdio_uart(blebutton 0)
target_include_directories(blebutton PRIVATE
${CMAKE_CURRENT_LIST_DIR}
)
pico_btstack_make_gatt_header(blebutton PRIVATE "${CMAKE_CURRENT_LIST_DIR}/blebutton.gatt")

pico_add_extra_outputs(blebutton)
