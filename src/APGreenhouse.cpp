//
// Created by David Trotz on 5/29/16.
//

#include "APGreenhouse.h"
#include "APSimpleJSONQuery.h"
#include "APSimpleSQL.h"
#include "APSQLException.h"
#include <sstream>
#include <string.h>
#include <json/json.h>

APGreenhouse::APGreenhouse(Json::Value& config) {
    _jsonQueryObj = new APSimpleJSONQuery();
    std::string databaseFile = config["sqlite3_file"].asString();
    _sqlDb = new APSimpleSQL(databaseFile);
    _config = config;
}

APGreenhouse::~APGreenhouse() {
    delete _jsonQueryObj;
    delete _sqlDb;
}

void APGreenhouse::InitializeSQLTables() {
    // Create the needed tables
    if (_sqlDb != NULL) {
        _sqlDb->BeginTransaction();
        try{
            _sqlDb->DoSQL("CREATE TABLE IF NOT EXISTS gh_data_points (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, timestamp integer, synchronized integer DEFAULT 0);");
            _sqlDb->DoSQL("CREATE INDEX IF NOT EXISTS index_gh_data_points_on_timestamp ON gh_data_points (timestamp);");

            _sqlDb->DoSQL("CREATE TABLE IF NOT EXISTS gh_sensor_data (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, sensor_id integer, temperature float, humidity float, gh_data_point_id integer);");
            _sqlDb->DoSQL("CREATE INDEX IF NOT EXISTS index_gh_sensor_data_on_gh_data_point_id ON gh_sensor_data (gh_data_point_id);");

            _sqlDb->DoSQL("CREATE TABLE IF NOT EXISTS gh_system_data (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, soc_temperature float, wlan0_link_quality float, wlan0_signal_level integer, storage_total_size integer,storage_used integer, storage_avail integer, gh_data_point_id integer);");
            _sqlDb->DoSQL("CREATE INDEX IF NOT EXISTS index_gh_system_data_on_gh_data_point_id ON gh_system_data(gh_data_point_id);");

            _sqlDb->DoSQL("CREATE TABLE IF NOT EXISTS gh_image_data (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, filename text, gh_data_point_id integer);");
            _sqlDb->DoSQL("CREATE INDEX IF NOT EXISTS index_gh_image_data_on_gh_data_point_id ON gh_image_data(gh_data_point_id);");
        } catch (APSQLException& e) {
            _sqlDb->RollbackTransaction();
            throw e;
        }

        _sqlDb->EndTransaction();
    }
}

void APGreenhouse::GetLatestSensorData() {
    time_t started_at = time(NULL);
    int afterTimestamp = -1;
    time_t maxTimestamp = _getMaxTimestampDataPoint();
    int beginCount = _getCurrentDataPointCount();
    const char* messagePrefix = "| Getting greenhouse and local sensor data since [%s]:";

    // Convert the timestamp into a readable format
    struct tm localMaxTimestamp;
    char readableTime[64];
    (void) localtime_r(&maxTimestamp, &localMaxTimestamp);

    if (strftime(readableTime, sizeof(readableTime), "%D %R", &localMaxTimestamp) == 0) {
        (void) fprintf(stderr,  "ERROR: timestamp conversion failed!\n");
        strncpy(readableTime, "UNKNOWN", sizeof(readableTime));
    }

    fprintf(stdout, messagePrefix, readableTime);
    while (maxTimestamp > afterTimestamp) {
        afterTimestamp = maxTimestamp;
        char *response = _getUpdateFeed(afterTimestamp, 25);
        if (response != NULL && strlen(response) > 0) {
            _parseJSONResponse(response);
        } else {
            break;
        }

        maxTimestamp = _getMaxTimestampDataPoint();
        if (time(NULL) - started_at > 60) {
            // We are taking too long to fetch so we will break and let others have some time.
            break;
        }
    }
    // Let's print some useful statistics about what just happened.
    int endCount = _getCurrentDataPointCount();
    int delta = endCount - beginCount;
    int buffLen = 128;
    char *messagePostfix = (char*)calloc(buffLen, sizeof(char));
    char *message = (char*)calloc(buffLen, sizeof(char));
    char *spaces = (char*)calloc(buffLen, sizeof(char));

    if (delta == 1) {
        strncpy(messagePostfix, "[1 DATA POINT FETCHED] |\n", buffLen);
    } else if (delta > 1) {
        snprintf(messagePostfix, buffLen, "[%d DATA POINTS FETCHED] |\n", delta);
    } else {
        strncpy(messagePostfix, "[NO DATA AVAILABLE] |\n", buffLen);
    }

    int totalSpaces = 89 - (strlen(messagePrefix) + strlen(messagePostfix));
    for (int i = 0; i < totalSpaces; ++i) {
        strcat(spaces, " ");
    }
    fprintf(stdout, "%s%s", spaces, messagePostfix);

}

time_t APGreenhouse::_getMaxTimestampDataPoint() {
    time_t maxTimestampDataPoint = 0;
    _sqlDb->BeginSelect("SELECT MAX(timestamp) FROM gh_data_points");
    if (_sqlDb->StepSelect()) {
        maxTimestampDataPoint = (time_t)_sqlDb->GetColAsInt64(0);
    }
    _sqlDb->EndSelect();
    return maxTimestampDataPoint;
}

int APGreenhouse::_getCurrentDataPointCount() {
    int count = 0;
    _sqlDb->BeginSelect("SELECT COUNT(id) FROM gh_data_points");
    if (_sqlDb->StepSelect()) {
        count = _sqlDb->GetColAsInt64(0);
    }
    _sqlDb->EndSelect();
    return count;
}

char* APGreenhouse::_getUpdateFeed(int afterTimestamp, int maxResults) {
    char url[2048];
    snprintf(url, sizeof(url), "http://192.168.0.190/data_points.json?after_timestamp=%d&limit=%d", afterTimestamp, maxResults);
    JSONResponseStructType response = _jsonQueryObj->GetJSONResponseFromURL(url);
    return (response.size > 0 && response.success) ? response.buffer : NULL;
}

void APGreenhouse::_parseJSONResponse(char *response) {
    // Prep the JSON object...
    std::stringstream s;
    s << response;
    Json::Value root;
    s >> root;

    std::vector<APKeyValuePair*> *pairs = NULL;
    APKeyValuePair* pair;
    int64_t gh_data_point_id;
    _sqlDb->BeginTransaction();
    try {
        for (const Json::Value& json : root["data_points"]) {
            pairs = new std::vector<APKeyValuePair*>();
            // Timestamp
            if (json["id"] != Json::Value::null && json["timestamp"] != Json::Value::null) {
                gh_data_point_id = (int64_t)json["id"].asInt64();
                pair = new APKeyValuePair("id", gh_data_point_id);
                pairs->push_back(pair);

                int64_t timestamp = json["timestamp"].asInt64();
                pair = new APKeyValuePair("timestamp", timestamp);
                pairs->push_back(pair);
                if (json["synchronized"] != Json::Value::null) {
                    pair = new APKeyValuePair("synchronized", json["synchronized"].asInt64());
                    pairs->push_back(pair);
                }

                int64_t lastId = _sqlDb->DoInsert("gh_data_points", pairs);
                _freeVectorAndData(pairs);

                if(lastId == gh_data_point_id) {
                    // Try to parse any sensor data
                    for (const Json::Value& sensor_datum : json["sensor_data"]) {
                        if (sensor_datum["id"] != Json::Value::null && sensor_datum["sensor_id"] != Json::Value::null) {
                            pairs = new std::vector<APKeyValuePair*>();

                            pair = new APKeyValuePair("id", sensor_datum["id"].asInt64());
                            pairs->push_back(pair);
                            pair = new APKeyValuePair("sensor_id", sensor_datum["sensor_id"].asInt64());
                            pairs->push_back(pair);

                            if (sensor_datum["temperature"] != Json::Value::null) {
                                pair = new APKeyValuePair("temperature", sensor_datum["temperature"].asDouble());
                                pairs->push_back(pair);
                            }

                            if (sensor_datum["humidity"] != Json::Value::null) {
                                pair = new APKeyValuePair("humidity", sensor_datum["humidity"].asDouble());
                                pairs->push_back(pair);
                            }

                            pair = new APKeyValuePair("gh_data_point_id", gh_data_point_id);
                            pairs->push_back(pair);

                            _sqlDb->DoInsert("gh_sensor_data", pairs);
                            _freeVectorAndData(pairs);
                        }
                    }
                    // Try and parse system datum
                    Json::Value system_datum = json["system_datum"];
                    if (system_datum != Json::Value::null && system_datum["id"] != Json::Value::null) {
                        pairs = new std::vector<APKeyValuePair*>();

                        pair = new APKeyValuePair("id", system_datum["id"].asInt64());
                        pairs->push_back(pair);

                        if (system_datum["soc_temperature"] != Json::Value::null) {
                            pair = new APKeyValuePair("soc_temperature", system_datum["soc_temperature"].asDouble());
                            pairs->push_back(pair);
                        }

                        if (system_datum["wlan0_link_quality"] != Json::Value::null) {
                            pair = new APKeyValuePair("wlan0_link_quality", system_datum["wlan0_link_quality"].asDouble());
                            pairs->push_back(pair);
                        }

                        if (system_datum["wlan0_signal_level"] != Json::Value::null) {
                            pair = new APKeyValuePair("wlan0_signal_level", system_datum["wlan0_signal_level"].asInt64());
                            pairs->push_back(pair);
                        }

                        if (system_datum["storage_total_size"] != Json::Value::null) {
                            pair = new APKeyValuePair("storage_total_size", system_datum["storage_total_size"].asInt64());
                            pairs->push_back(pair);
                        }

                        if (system_datum["storage_used"] != Json::Value::null) {
                            pair = new APKeyValuePair("storage_used", system_datum["storage_used"].asInt64());
                            pairs->push_back(pair);
                        }

                        if (system_datum["storage_avail"] != Json::Value::null) {
                            pair = new APKeyValuePair("storage_avail", system_datum["storage_avail"].asInt64());
                            pairs->push_back(pair);
                        }

                        pair = new APKeyValuePair("gh_data_point_id", gh_data_point_id);
                        pairs->push_back(pair);

                        _sqlDb->DoInsert("gh_system_data", pairs);
                        _freeVectorAndData(pairs);
                    }

                    // Try and parse image data
                    for (const Json::Value& image_datum : json["image_data"]) {
                        if (image_datum["id"] != Json::Value::null && image_datum["filename"] != Json::Value::null) {
                            pairs = new std::vector<APKeyValuePair*>();

                            pair = new APKeyValuePair("id", image_datum["id"].asInt64());
                            pairs->push_back(pair);
                            pair = new APKeyValuePair("filename", image_datum["filename"].asString());
                            pairs->push_back(pair);

                            pair = new APKeyValuePair("gh_data_point_id", gh_data_point_id);
                            pairs->push_back(pair);

                            _sqlDb->DoInsert("gh_image_data", pairs);
                            _freeVectorAndData(pairs);
                        }
                    }
                } else {
                    throw APException("Insert gh_data_point mismatched rowid!");
                }


            } else {
                if (json["id"] != Json::Value::null) {
                    throw APException("Invalid Greenhouse Json Feed: Missing id.");
                } else {
                    throw APException("Invalid Greenhouse Json Feed: Missing timestamp.");
                }
            }
        }
    } catch (APSQLException& e) {
        fprintf(stdout, "Failed with exception: %s", e.what());
        _sqlDb->RollbackTransaction();
        throw e;
    }
    _sqlDb->EndTransaction();
}

void APGreenhouse::_freeVectorAndData(std::vector<APKeyValuePair*>* pairs) {
    for (std::vector<APKeyValuePair *>::iterator it = pairs->begin(); it != pairs->end(); ++it) {
        delete (*it);
    }
    delete pairs;
}
