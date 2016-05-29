//
// Created by David Trotz on 5/29/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
#define GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
class APSimpleJSONQuery;

class APGreenhouse {
public:
    APGreenhouse();
    ~APGreenhouse();

    void UpdateGreenhouseSensorData(APSimpleSQL* db, Json::Value& config);
    void InitializeSQLTables(APSimpleSQL* db);
private:
    APSimpleJSONQuery* _jsonQueryObj;
};


#endif //GREENHOUSE_DATA_SERVER_APGREENHOUSE_H
