cmake_minimum_required(VERSION 3.0)
project(greenhouse_data_server)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
        src/main.cpp
        src/APWeatherDataManager.cpp src/APWeatherDataManager.h src/util/APSimpleSQL.cpp src/util/APSimpleSQL.h src/weather_sources/APWeatherSource.cpp src/weather_sources/APWeatherSource.h src/weather_sources/APOpenWeatherMap.cpp src/weather_sources/APOpenWeatherMap.h src/weather_sources/APDarkSkyForecastIO.cpp src/weather_sources/APDarkSkyForecastIO.h src/util/APSimpleJSONQuery.cpp src/util/APSimpleJSONQuery.h src/util/APException.cpp src/util/APException.h src/util/APSQLException.cpp src/util/APSQLException.h src/util/APKeyValuePair.cpp src/util/APKeyValuePair.h src/APGreenhouse.cpp src/APGreenhouse.h src/APImageTransferAgent.cpp src/APImageTransferAgent.h)

add_executable(greenhouse_data_server ${SOURCE_FILES})
include_directories(src/util src/weather_sources)
link_directories(/use/local/lib)
target_include_directories(greenhouse_data_server PUBLIC /usr/include/jsoncpp/)
target_link_libraries(greenhouse_data_server sqlite3)
target_link_libraries(greenhouse_data_server jsoncpp)
target_link_libraries(greenhouse_data_server curl)
