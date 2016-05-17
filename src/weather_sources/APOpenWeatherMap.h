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
    char* _getCurrentConditions(Json::Value& config);
    void _parseJSONResponse(char *response, APSimpleSQL *db, bool live_condition);
    void _parseWeatherInfo(Json::Value& json, APSimpleSQL *db, bool live_condition, int64_t city_id, time_t now, int daysSinceEpoch);
};


#endif //GREENHOUSE_DATA_SERVER_APOPENWEATHERMAP_H
