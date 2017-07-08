//
// Created by David Trotz on 5/29/16.
//
#include <json/json.h>

#ifndef GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
#define GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
class APSimpleJSONQuery;
class APSimpleSQL;
class APKeyValuePair;

class APGreenhouse {
public:
    APGreenhouse(Json::Value& config);
    ~APGreenhouse();

    void GetLatestSensorData();
    void InitializeSQLTables();
private:
    char* _getUpdateFeed(int afterTimestamp, int maxResults);
    void _parseJSONResponse(char *response);
    time_t _getMaxTimestampDataPoint();
    int _getCurrentDataPointCount();
    void _freeVectorAndData(std::vector<APKeyValuePair*>* pairs);
    APSimpleSQL* _sqlDb;
    APSimpleJSONQuery* _jsonQueryObj;
    Json::Value _config;
};


#endif //GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
