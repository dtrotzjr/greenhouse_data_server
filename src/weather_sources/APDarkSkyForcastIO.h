//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APDARKSKYFORCASTIO_H
#define GREENHOUSE_DATA_SERVER_APDARKSKYFORCASTIO_H
#include "APWeatherSource.h"

class APSimpleSQL;

class APDarkSkyForcastIO : public APWeatherSource {
public:
    virtual void InitializeSQLTables(APSimpleSQL* db);
    virtual void UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config);
protected:
    char* _getForecast(Json::Value& config);
    void _parseJSONResponse(char *response, APSimpleSQL *db, bool live_condition);

};


#endif //GREENHOUSE_DATA_SERVER_APDARKSKYFORCASTIO_H
