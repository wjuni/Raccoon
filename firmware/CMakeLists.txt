cmake_minimum_required(VERSION 2.8.4)
set(CMAKE_TOOLCHAIN_FILE ${CMAKE_SOURCE_DIR}/cmake/ArduinoToolchain.cmake)
set(PROJECT_NAME firmware)
project(${PROJECT_NAME})

set(${CMAKE_PROJECT_NAME}_SKETCH firmware.ino)
set(${CMAKE_PROJECT_NAME}_SRCS
        Adafruit_GPS.cpp
        Compass.cpp
        GpsModule.cpp
        HMC5883L.cpp
        PktProtocol.cpp
        SerialComm.cpp
        StepMotor.cpp)

#### Uncomment below additional settings as needed.
set(${CMAKE_PROJECT_NAME}_BOARD mega)
# set(${CMAKE_PROJECT_NAME}_PORT /dev/ttyACM0)
set(mega.build.mcu atmega2560)
# set(mega.upload.protocol wiring)
# set(mega.upload.speed 115200)
message("Start Generation")

generate_arduino_firmware(${CMAKE_PROJECT_NAME})
