#include <iostream>
#include "APWeatherDataManager.h"

#define CURRENT_WEATHER_URL "http://api.openweathermap.org/data/2.5/weather"
#define FIVE_DAY_FORECAST_URL = "http://api.openweathermap.org/data/2.5/forecast"

int main(int argc, char* argv[]) 
{
    if (argc == 2) {
        APWeatherDataManager* wm = new APWeatherDataManager(argv[1]);
        wm->GetLatestWeatherData();

        return 0;
    }
    std::cout << "Usage " + std::string(argv[1]) + " <path to config.json>" << std::endl;
    return -1;
}
