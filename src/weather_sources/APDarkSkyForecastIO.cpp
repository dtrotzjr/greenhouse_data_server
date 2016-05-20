//
// Created by David Trotz on 5/15/16.
//
#include <json/json.h>
#include "APDarkSkyForecastIO.h"
#include "APSimpleJSONQuery.h"
#include "APSimpleSQL.h"
#include "APSQLException.h"
#include <sstream>

void APDarkSkyForecastIO::InitializeSQLTables(APSimpleSQL* db) {
    // Create the needed tables
    if (db != NULL) {
        db->BeginTransaction();
        try{
            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_forecast (id INTEGER PRIMARY KEY AUTOINCREMENT, master_timestamp INTEGER, type_id INTEGER REFERENCES fio_forecast_type(id) ON DELETE NO ACTION, summary_id INTEGER REFERENCES fio_summary(id) ON DELETE SET NULL, icon_id INTEGER REFERENCES fio_icon(id) ON DELETE NO ACTION);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS fio_forecast_time_idx ON fio_forecast(master_timestamp);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_forecast_type (id INTEGER PRIMARY KEY AUTOINCREMENT, type TEXT UNIQUE);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS fio_forecast_type_idx ON fio_forecast_type(type);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_icon (id INTEGER PRIMARY KEY AUTOINCREMENT, icon TEXT UNIQUE);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS fio_icon_idx ON fio_icon(icon);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_summary (id INTEGER PRIMARY KEY AUTOINCREMENT, summary TEXT UNIQUE);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS fio_summary_idx ON fio_summary(summary);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_data (id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, nearest_storm_distance REAL, nearest_storm_bearing REAL, precip_intensity REAL, precip_intensity_max REAL, precip_intensity_max_time INTEGER, precip_intensity_error REAL, precip_accumulation REAL, precip_probability REAL, precip_type TEXT, temperature REAL, temperature_min REAL,temperature_min_time INTEGER, temperature_max REAL,temperature_max_time INTEGER, apparent_temperature REAL, apparent_temperature_min REAL, apparent_temperature_min_time INTEGER, apparent_temperature_max REAL, apparent_temperature_max_time INTEGER, dew_point REAL, wind_speed REAL, wind_bearing REAL, cloud_cover REAL, humidity REAL, pressure REAL, visibility REAL,  ozone REAL, fio_forecast_id INTEGER REFERENCES fio_forecast(id) ON DELETE CASCADE, fio_day_info_id INTEGER REFERENCES fio_day_info(id) ON DELETE SET NULL, summary_id INTEGER REFERENCES fio_summary(id) ON DELETE NO ACTION, icon_id INTEGER REFERENCES fio_icon(id) ON DELETE NO ACTION);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS fio_data_time_idx ON fio_data(time);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_day_info(id INTEGER PRIMARY KEY, sunrise INTEGER, sunset INTEGER, moon_phase REAL);");

            // Populate the forecast types
            size_t buffLen = 512;
            char buff[buffLen];
            if (!db->RowExists("fio_forecast_type", (int64_t)APForecastType::Currently)) {
                snprintf(buff, buffLen, "INSERT INTO fio_forecast_type (id, type) VALUES (%lld, '%s');", APForecastType::Currently, AP_FIO_TYPE_CURRENTLY);
                db->DoInsert(buff);
            }
            if (!db->RowExists("fio_forecast_type", (int64_t)APForecastType::Minutely)) {
                snprintf(buff, buffLen, "INSERT INTO fio_forecast_type (id, type) VALUES (%lld, '%s');",
                         APForecastType::Minutely, AP_FIO_TYPE_MINUTELY);
                db->DoInsert(buff);
            }
            if (!db->RowExists("fio_forecast_type", (int64_t)APForecastType::Hourly)) {
                snprintf(buff, buffLen, "INSERT INTO fio_forecast_type (id, type) VALUES (%lld, '%s');",
                         APForecastType::Hourly, AP_FIO_TYPE_HOURLY);
                db->DoInsert(buff);
            }
            if (!db->RowExists("fio_forecast_type", (int64_t)APForecastType::Daily)) {
                snprintf(buff, buffLen, "INSERT INTO fio_forecast_type (id, type) VALUES (%lld, '%s');",
                         APForecastType::Daily, AP_FIO_TYPE_DAILY);
                db->DoInsert(buff);
            }
        } catch (APSQLException& e) {
            db->RollbackTransaction();
            throw e;
        }

        db->EndTransaction();
    }
}

void APDarkSkyForecastIO::UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config) {
    if (db != NULL) {
        printf("Getting Forecast.io weather conditions...\n");
        char *response = _getJSONFromForecastIOService(config);
        printf("Parsing Forecast.io weather conditions...\n");
        if (response != NULL) {
            _parseJSONResponse(response, db, true);
        }
        printf("DONE: Forecast.io weather conditions...\n");

    }
}

char* APDarkSkyForecastIO::_getJSONFromForecastIOService(Json::Value &config) {
    char* response = NULL;
    std::string lat = config["latitude"].asString();
    std::string lon = config["longitude"].asString();
    std::string apikey = config["forcast_io_api_key"].asString();
    if (lat.length() > 0 && lon.length() > 0 && apikey.length() > 0) {
        char url[2048];
        snprintf(url, sizeof(url), "https://api.forecast.io/forecast/%s/%s,%s", apikey.c_str(), lat.c_str(), lon.c_str());

        response = _jsonQueryObj->GetJSONResponseFromURL(url);
    }
    return response;
}

void APDarkSkyForecastIO::_parseJSONResponse(char *response, APSimpleSQL *db, bool live_condition) {
    int buffSize = 16384;
    char buff[buffSize];
    // Prep the JSON object...
    std::stringstream s;
    s << response;
    Json::Value json;
    s >> json;
    // Store the values in the database
    db->BeginTransaction();
    try {
        // Currently Data
        Json::Value currently = json[AP_FIO_TYPE_CURRENTLY];
        if (currently != Json::Value::null && currently["time"] != Json::Value::null) {
            // time
            int64_t masterTimestamp = currently["time"].asInt64();
            _parseForecastJSONEntry(db, masterTimestamp, currently, APForecastType::Currently);

            // Daily
            time_t now = time(NULL);
            struct tm *today = localtime(&now);
            today->tm_hour = 0;
            today->tm_min = 0;
            today->tm_sec = 0;
            int64_t startOfToday = (int64_t)mktime(today);
            snprintf(buff, buffSize, "SELECT id FROM fio_forecast WHERE master_timestamp > %lld AND type_id == %lld LIMIT 1", startOfToday, (int64_t)APForecastType::Daily);
            db->BeginSelect(buff);
            bool needDaily = !db->StepSelect();
            db->EndSelect();

            Json::Value daily = json[AP_FIO_TYPE_DAILY];
            if (needDaily && daily != Json::Value::null) {
                _parseForecastJSONEntry(db, masterTimestamp, daily, APForecastType::Daily);
            }

        }
    } catch (APSQLException& e) {
        db->RollbackTransaction();
        throw e;
    }

    db->EndTransaction();
}

void APDarkSkyForecastIO::_parseForecastJSONEntry(APSimpleSQL *db, int64_t masterTimestamp, Json::Value &json, APForecastType forecastType) {
    // Prep and store the main forecast object
    std::vector<APKeyValuePair*>* pairs = new std::vector<APKeyValuePair*>();
    // type
    APKeyValuePair* pair = new APKeyValuePair("type_id", (int64_t)forecastType);
    pairs->push_back(pair);

    pair = new APKeyValuePair("master_timestamp", masterTimestamp);
    pairs->push_back(pair);
    // summary
    if (json["summary"] != Json::Value::null) {
        int64_t summary_id = _getOrCreateSummaryId(db, json["summary"].asString());
        pair = new APKeyValuePair("summary_id", summary_id);
        pairs->push_back(pair);
    }
    // icon
    if (json["icon"] != Json::Value::null) {
        int64_t icon_id = _getOrCreateIconId(db, json["icon"].asString());
        pair = new APKeyValuePair("icon_id", icon_id);
        pairs->push_back(pair);
    }
    int64_t forecast_id = db->DoInsert("fio_forecast", pairs);
    _freeVectorAndData(pairs);


    Json::Value datum = json["data"];
    if (datum != Json::Value::null) {
        for (Json::Value& data : datum) {
            pairs = new std::vector<APKeyValuePair*>();
            _parseDataJSONEntry(db, forecast_id, masterTimestamp, data, pairs);
            int64_t data_id = db->DoInsert("fio_data", pairs);
            // Cleanup
            _freeVectorAndData(pairs);
        }
    } else {
        // Currently stores the data at the same level as the forecast entry
        pairs = new std::vector<APKeyValuePair*>();
        _parseDataJSONEntry(db, forecast_id, masterTimestamp, json, pairs);
        int64_t data_id = db->DoInsert("fio_data", pairs);
        _freeVectorAndData(pairs);
    }
}

int64_t APDarkSkyForecastIO::_getOrCreateIconId(APSimpleSQL *db, std::string iconName) {
    return _getOrCreateIdForColumnInTableWithValue(db, "fio_icon", "icon", iconName);
}
int64_t APDarkSkyForecastIO::_getOrCreateSummaryId(APSimpleSQL *db, std::string summaryText) {
    return _getOrCreateIdForColumnInTableWithValue(db, "fio_summary", "summary", summaryText);
}

int64_t APDarkSkyForecastIO::_getOrCreateIdForColumnInTableWithValue(APSimpleSQL *db, std::string table,
                                                                     std::string col, std::string value) {
    int64_t rowId = -1;
    size_t buffLen = 512;
    char buff[buffLen];
    snprintf(buff, buffLen, "SELECT id FROM '%s' WHERE %s = '%s'", table.c_str(), col.c_str(), value.c_str());

    db->BeginSelect(buff);
    if(db->StepSelect()) {
        rowId = db->GetColAsInt64(0);
    } else {
        snprintf(buff, buffLen, "INSERT INTO %s (%s) VALUES ('%s')", table.c_str(), col.c_str(), value.c_str());
        rowId = db->DoInsert(buff);
    }
    db->EndSelect();
    if (rowId == -1) {
        throw APException("Failed to get id for row.");
    }
    return rowId;
}

void APDarkSkyForecastIO::_freeVectorAndData(std::vector<APKeyValuePair*>* pairs) {
    for (std::vector<APKeyValuePair *>::iterator it = pairs->begin(); it != pairs->end(); ++it) {
        delete (*it);
    }
    delete pairs;
}

void APDarkSkyForecastIO::_parseDataJSONEntry(APSimpleSQL *db, int64_t forecast_id, int64_t masterTimestamp,
                                              Json::Value &json, std::vector<APKeyValuePair *> *pairs) {
    APKeyValuePair* pair;
    // Timestamp
    if (json["time"] != Json::Value::null) {
        pair = new APKeyValuePair("time", json["time"].asInt64());
        pairs->push_back(pair);
    } else {
        pair = new APKeyValuePair("time", masterTimestamp);
        pairs->push_back(pair);
    }
    // summary
    if (json["summary"] != Json::Value::null) {
        int64_t summary_id = _getOrCreateSummaryId(db, json["summary"].asString());
        pair = new APKeyValuePair("summary_id", summary_id);
        pairs->push_back(pair);
    }
    // icon
    if (json["icon"] != Json::Value::null) {
        int64_t icon_id = _getOrCreateIconId(db, json["icon"].asString());
        pair = new APKeyValuePair("icon_id", icon_id);
        pairs->push_back(pair);
    }
    // nearestStormDistance
    if (json["nearestStormDistance"] != Json::Value::null) {
        pair = new APKeyValuePair("nearest_storm_distance", json["nearestStormDistance"].asDouble());
        pairs->push_back(pair);
    }
    // nearestStormBearing
    if (json["nearestStormBearing"] != Json::Value::null) {
        pair = new APKeyValuePair("nearest_storm_bearing", json["nearestStormBearing"].asDouble());
        pairs->push_back(pair);
    }
    // precipIntensity
    if (json["precipIntensity"] != Json::Value::null) {
        pair = new APKeyValuePair("precip_intensity", json["precipIntensity"].asDouble());
        pairs->push_back(pair);
    }
    // precipIntensityMax
    if (json["precipIntensityMax"] != Json::Value::null) {
        pair = new APKeyValuePair("precip_intensity_max", json["precipIntensityMax"].asDouble());
        pairs->push_back(pair);
    }
    // precipIntensityMaxTime
    if (json["precipIntensityMaxTime"] != Json::Value::null) {
        pair = new APKeyValuePair("precip_intensity_max_time", json["precipIntensityMaxTime"].asInt64());
        pairs->push_back(pair);
    }
    // precipProbability
    if (json["precipProbability"] != Json::Value::null) {
        pair = new APKeyValuePair("precip_probability", json["precipProbability"].asDouble());
        pairs->push_back(pair);
    }
    // precipType
    if (json["precipType"] != Json::Value::null) {
        pair = new APKeyValuePair("precip_type", json["precipType"].asString());
        pairs->push_back(pair);
    }
    // precipAccumulation
    if (json["precipAccumulation"] != Json::Value::null) {
        pair = new APKeyValuePair("precip_accumulation", json["precipAccumulation"].asDouble());
        pairs->push_back(pair);
    }
    // temperature
    if (json["temperature"] != Json::Value::null) {
        pair = new APKeyValuePair("temperature", json["temperature"].asDouble());
        pairs->push_back(pair);
    }
    // temperatureMin
    if (json["temperatureMin"] != Json::Value::null) {
        pair = new APKeyValuePair("temperature_min", json["temperatureMin"].asDouble());
        pairs->push_back(pair);
    }
    // temperatureMinTime
    if (json["temperatureMinTime"] != Json::Value::null) {
        pair = new APKeyValuePair("temperature_min_time", json["temperatureMinTime"].asInt64());
        pairs->push_back(pair);
    }
    // temperatureMax
    if (json["temperatureMax"] != Json::Value::null) {
        pair = new APKeyValuePair("temperature_max", json["temperatureMax"].asDouble());
        pairs->push_back(pair);
    }
    // temperatureMaxTime
    if (json["temperatureMaxTime"] != Json::Value::null) {
        pair = new APKeyValuePair("temperature_max_time", json["temperatureMaxTime"].asInt64());
        pairs->push_back(pair);
    }
    // apparentTemperature
    if (json["apparentTemperature"] != Json::Value::null) {
        pair = new APKeyValuePair("apparent_temperature", json["apparentTemperature"].asDouble());
        pairs->push_back(pair);
    }
    // apparentTemperatureMin
    if (json["apparentTemperatureMin"] != Json::Value::null) {
        pair = new APKeyValuePair("apparent_temperature_min", json["apparentTemperatureMin"].asDouble());
        pairs->push_back(pair);
    }
    // apparentTemperatureMinTime
    if (json["apparentTemperatureMinTime"] != Json::Value::null) {
        pair = new APKeyValuePair("apparent_temperature_min_time", json["apparentTemperatureMinTime"].asInt64());
        pairs->push_back(pair);
    }
    // apparentTemperatureMax
    if (json["apparentTemperatureMax"] != Json::Value::null) {
        pair = new APKeyValuePair("apparent_temperature_max", json["apparentTemperatureMax"].asDouble());
        pairs->push_back(pair);
    }
    // apparentTemperatureMaxTime
    if (json["apparentTemperatureMaxTime"] != Json::Value::null) {
        pair = new APKeyValuePair("apparent_temperature_max_time", json["apparentTemperatureMaxTime"].asInt64());
        pairs->push_back(pair);
    }
    // dewPoint
    if (json["dewPoint"] != Json::Value::null) {
        pair = new APKeyValuePair("dew_point", json["dewPoint"].asDouble());
        pairs->push_back(pair);
    }
    // windSpeed
    if (json["windSpeed"] != Json::Value::null) {
        pair = new APKeyValuePair("wind_speed", json["windSpeed"].asDouble());
        pairs->push_back(pair);
    }
    // windBearing
    if (json["windBearing"] != Json::Value::null) {
        pair = new APKeyValuePair("wind_bearing", json["windBearing"].asDouble());
        pairs->push_back(pair);
    }
    // cloudCover
    if (json["cloudCover"] != Json::Value::null) {
        pair = new APKeyValuePair("cloud_cover", json["cloudCover"].asDouble());
        pairs->push_back(pair);
    }
    // humidity
    if (json["humidity"] != Json::Value::null) {
        pair = new APKeyValuePair("humidity", json["humidity"].asDouble());
        pairs->push_back(pair);
    }
    // pressure
    if (json["pressure"] != Json::Value::null) {
        pair = new APKeyValuePair("pressure", json["pressure"].asDouble());
        pairs->push_back(pair);
    }
    // visibility
    if (json["visibility"] != Json::Value::null) {
        pair = new APKeyValuePair("visibility", json["visibility"].asDouble());
        pairs->push_back(pair);
    }
    // ozone
    if (json["ozone"] != Json::Value::null) {
        pair = new APKeyValuePair("ozone", json["ozone"].asDouble());
        pairs->push_back(pair);
    }
    pair = new APKeyValuePair("fio_forecast_id", forecast_id);
    pairs->push_back(pair);
}