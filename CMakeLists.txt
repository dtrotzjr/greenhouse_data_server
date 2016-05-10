cmake_minimum_required(VERSION 3.5)
project(greenhouse_data_server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    src/main.c
    )

add_executable(greenhouse_data_server ${SOURCE_FILES})