//
// Created by David Trotz on 5/29/16.
//
#include <json/json.h>

#ifndef GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
#define GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
class APSimpleJSONQuery;
class APSimpleSQL;

class APGreenhouse {
public:
    APGreenhouse(APSimpleSQL* sqlDb, Json::Value& config);
    ~APGreenhouse();

    void UpdateGreenhouseSensorData();
    void InitializeSQLTables();
private:
    APSimpleSQL* _sqlDb;
    APSimpleJSONQuery* _jsonQueryObj;
    Json::Value _config;
};


#endif //GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
