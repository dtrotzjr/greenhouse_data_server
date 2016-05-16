//
// Created by David Trotz on 5/9/16.
//
#include <string>
#include <sqlite3.h>
#include <json/json.h>
#include <APWeatherSource.h>
#include <vector>

#ifndef GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
#define GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
class APSimpleSQL;

class APWeatherDataManager {
public:
    APWeatherDataManager(std::string configPath);
    virtual ~APWeatherDataManager();

    void GetLatestWeatherData();
protected:
    APSimpleSQL *_sqlDb;
    Json::Value _config;
    std::vector<APWeatherSource*> *_sources;
};


#endif //GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
