cmake_minimum_required(VERSION 3.8)
project(firmware)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        Adafruit_GPS.cpp
        Adafruit_GPS.h
        Compass.cpp
        Compass.h
        debug.h
        firmware.ino
        GpsModule.cpp
        GpsModule.h
        HMC5883L.cpp
        HMC5883L.h
        PktProtocol.cpp
        PktProtocol.h
        SerialComm.cpp
        SerialComm.h
        StepMotor.cpp
        StepMotor.h)

add_executable(firmware ${SOURCE_FILES})