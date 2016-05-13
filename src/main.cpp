#include <iostream>
#include "APWeatherData.h"

#define CURRENT_WEATHER_URL "http://api.openweathermap.org/data/2.5/weather"
#define FIVE_DAY_FORECAST_URL = "http://api.openweathermap.org/data/2.5/forecast"

int main(int argc, char* argv[]) 
{
    if (argc == 2) {
        APWeatherData* wd = new APWeatherData();
        wd->init(argv[1]);
        char* response = wd->Get5DayForecast();
        wd->ParseAndStore5DayForecastResponse(response);
        return 0;
    }
    std::cout << "Usage " + std::string(argv[1]) + " <path to config.json>" << std::endl;
    return -1;
}
