cmake_minimum_required(VERSION 3.6)
project(UDPEcho_master)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp src/Packet.h)
add_executable(UDPEcho_master ${SOURCE_FILES})