//
// Created by David Trotz on 5/9/16.
//
#include <string>
#include <sqlite3.h>
#include <json/json.h>
#ifndef GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
#define GREENHOUSE_DATA_SERVER_APWEATHERDATA_H


typedef struct MemoryStruct {
    char *memory;
    size_t size;
} MemoryStructType;

class APWeatherDataManager {
protected:
    sqlite3 *_db;
    Json::Value _config;
    MemoryStructType _curlResponseChunk;
public:
    APWeatherDataManager();
    virtual ~APWeatherDataManager();
    bool init(std::string configPath);

    char* Get5DayForecast();
    void ParseAndStore5DayForecastResponse(char* response);

protected:
    bool _beginTransation();
    bool _endTransation();
    bool _doSQL(const char* sql);

};


#endif //GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
