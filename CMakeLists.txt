cmake_minimum_required(VERSION 3.13)

# Set the Pico SDK path (update it with your own path)
set(PICO_SDK_PATH $ENV{PICO_SDK_PATH})

# Set the Pico board
set(PICO_BOARD pico_w)

set(CMAKE_BUILD_TYPE Debug)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


# Set the project name
set(PROJECT_NAME garage_door_opener)

# Include the Pico SDK. Must be before project()
include(${PICO_SDK_PATH}/pico_sdk_init.cmake)

# Set the project name
project(${PROJECT_NAME} C CXX ASM)

# Set the C/C++ standard
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 17)

# Initialize the Pico SDK
pico_sdk_init()

file(GLOB_RECURSE SRC_FILES "src/*.cpp")
file(GLOB MQTT_SOURCES "paho.mqtt.embedded-c/MQTTPacket/src/*.c")
# Add the project source files
add_executable(${PROJECT_NAME}
  ${SRC_FILES}
  ${MQTT_SOURCES}
        include/network_info.h
)


target_compile_options(${PROJECT_NAME} PRIVATE
  -Wall
  -Wextra
  -Wpedantic
)

target_include_directories(${PROJECT_NAME}
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    paho.mqtt.embedded-c/MQTTPacket/src
    paho.mqtt.embedded-c
)

# Link the Pico SDK libraries
target_link_libraries(${PROJECT_NAME}
    pico_stdlib
    hardware_pwm
    hardware_gpio
    hardware_i2c
    pico_cyw43_arch_lwip_poll
    pico_multicore
    # additional Pico SDK libraries
)

# Enable UART output (optional)
pico_enable_stdio_uart(${PROJECT_NAME} 1)

# Enable USB output (optional)
pico_enable_stdio_usb(${PROJECT_NAME} 1)

# Create map/bin/hex/uf2 files
pico_add_extra_outputs(${PROJECT_NAME})