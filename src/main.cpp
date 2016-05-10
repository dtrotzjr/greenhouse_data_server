#include <iostream>
#include <fstream>
#include <json/json.h>
#include "APWeatherData.h"

#define CURRENT_WEATHER_URL "http://api.openweathermap.org/data/2.5/weather"
#define FIVE_DAY_FORECAST_URL = "http://api.openweathermap.org/data/2.5/forecast"

int main(int argc, char* argv[]) 
{
    if (argc == 2) {
        Json::Value root;
        std::ifstream jsonFileStream;
        jsonFileStream.open(argv[1]);
        jsonFileStream >> root;

        APWeatherData* wd = new APWeatherData();
        wd->init(root["sqlite3_file"].asString());
        return 0;
    }
    std::cout << "Usage " + std::string(argv[1]) + " <path to config.json>" << std::endl;
    return -1;
}
