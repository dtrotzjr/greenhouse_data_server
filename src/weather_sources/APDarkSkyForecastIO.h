//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APDARKSKYFORCASTIO_H
#define GREENHOUSE_DATA_SERVER_APDARKSKYFORCASTIO_H
#include "APWeatherSource.h"
#include <vector>
class APSimpleSQL;
class APKeyValuePair;

class APDarkSkyForecastIO : public APWeatherSource {
public:
    virtual void InitializeSQLTables(APSimpleSQL* db);
    virtual void UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config);
protected:
    char* _getJSONFromForecastIOService(Json::Value& config);
    void _parseJSONResponse(char *response, APSimpleSQL *db, bool live_condition);
    void _parseForecastJSONEntry(APSimpleSQL* db, int64_t masterTimestamp, Json::Value& json);
    void _parseDataJSONEntry(APSimpleSQL* db, int64_t forecast_id, int64_t masterTimestamp, Json::Value& json, std::vector<APKeyValuePair*>* pairs);
    void _freeVectorAndData(std::vector<APKeyValuePair*>* pairs);
    int64_t _getOrCreateIconId(APSimpleSQL *db, std::string iconName);
    int64_t _getOrCreateSummaryId(APSimpleSQL *db, std::string summaryText);
    int64_t _getOrCreateIdForColumnInTableWithValue(APSimpleSQL *db, std::string table, std::string col,
                                                    std::string value);
};


#endif //GREENHOUSE_DATA_SERVER_APDARKSKYFORCASTIO_H
