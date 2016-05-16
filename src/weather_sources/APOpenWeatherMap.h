//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APOPENWEATHERMAP_H
#define GREENHOUSE_DATA_SERVER_APOPENWEATHERMAP_H
#include "APWeatherSource.h"
class APSimpleJSONQuery;

class APOpenWeatherMap : public APWeatherSource {
public:
    APOpenWeatherMap();
    virtual ~APOpenWeatherMap();

    virtual void InitializeSQLTables(APSimpleSQL* db);
    virtual void UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config);
private:
    APSimpleJSONQuery* _jsonQueryObj;

    char* _getForecast(Json::Value& config);
    void _parseForecastResponse(char *response, APSimpleSQL *db);
};


#endif //GREENHOUSE_DATA_SERVER_APOPENWEATHERMAP_H
