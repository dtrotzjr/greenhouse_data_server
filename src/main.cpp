

#include "APWeatherData.h"

#define CURRENT_WEATHER_URL "http://api.openweathermap.org/data/2.5/weather"
#define FIVE_DAY_FORECAST_URL = "http://api.openweathermap.org/data/2.5/forecast"

int main(int argc, char* argv[]) 
{
    APWeatherData* wd = new APWeatherData();
    wd->init("test.sqlite");
}