//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APWEATHERSOURCE_H
#define GREENHOUSE_DATA_SERVER_APWEATHERSOURCE_H

#include <json/value.h>
class APSimpleSQL;

class APWeatherSource {
public:
    APWeatherSource() {};
    virtual ~APWeatherSource() {};
    virtual void UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config) = 0;
    virtual void InitializeSQLTables(APSimpleSQL* db) = 0;
};


#endif //GREENHOUSE_DATA_SERVER_APWEATHERSOURCE_H
