//
// Created by David Trotz on 5/9/16.
//
#include <string>
#include <sqlite3.h>
#include <json/json.h>
#ifndef GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
#define GREENHOUSE_DATA_SERVER_APWEATHERDATA_H

class APWeatherData {
protected:
    sqlite3 *_db;
    Json::Value _config;
public:
    APWeatherData();
    virtual ~APWeatherData();
    bool init(std::string configPath);
};


#endif //GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
