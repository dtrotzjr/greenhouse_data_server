cmake_minimum_required(VERSION 3.0)
project(greenhouse_data_server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/main.cpp
        src/APWeatherData.cpp src/APWeatherData.h)

add_executable(greenhouse_data_server ${SOURCE_FILES})

link_directories(/use/local/lib)
target_link_libraries(greenhouse_data_server sqlite3)