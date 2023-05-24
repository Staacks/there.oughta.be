cmake_minimum_required(VERSION 3.13)

include(pico_sdk_import.cmake)

project(pico-controller C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(pico-controller)

target_sources(pico-controller PUBLIC
	${CMAKE_CURRENT_LIST_DIR}/pico-controller.c
	)

target_link_libraries(pico-controller PUBLIC pico_stdlib)

target_include_directories(pico-controller PUBLIC ${CMAKE_CURRENT_LIST_DIR})

pico_add_extra_outputs(pico-controller)

pico_enable_stdio_usb(pico-controller 1)
pico_enable_stdio_uart(pico-controller 0)
