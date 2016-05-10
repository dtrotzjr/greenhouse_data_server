//
// Created by David Trotz on 5/9/16.
//
#include <string>
#include <sqlite3.h>
#ifndef GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
#define GREENHOUSE_DATA_SERVER_APWEATHERDATA_H

class APWeatherData {
protected:
    sqlite3 *_db;
public:
    APWeatherData();
    virtual ~APWeatherData();
    bool init(std::string* dbPath);
};


#endif //GREENHOUSE_DATA_SERVER_APWEATHERDATA_H
