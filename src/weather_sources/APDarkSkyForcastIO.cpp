//
// Created by David Trotz on 5/15/16.
//
#include <json/json.h>
#include "APDarkSkyForcastIO.h"
#include "APSimpleJSONQuery.h"
#include "APSimpleSQL.h"
#include "APSQLException.h"
#include <sstream>

void APDarkSkyForcastIO::InitializeSQLTables(APSimpleSQL* db) {
    // Create the needed tables
    if (db != NULL) {
        db->BeginTransaction();
        try{
            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_forecast (id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, type TEXT, summary TEXT, icon TEXT);");
            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_data (id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, summary TEXT, icon TEXT, nearest_storm_distance REAL, nearest_storm_bearing REAL, precip_intensity REAL, precip_intensity_max REAL, precip_intensity_max_time INTEGER, precip_intensity_error REAL, precip_accumulation REAL, precip_probability REAL, precip_type TEXT, temperature REAL, temperature_min REAL,temperature_min_time INTEGER, temperature_max REAL,temperature_max_time INTEGER, apparent_temperature REAL, apparent_temperature_min REAL, apparent_temperature_min_time INTEGER, apparent_temperature_max REAL, apparent_temperature_max_time INTEGER, dew_point REAL, wind_speed REAL, wind_bearing REAL, cloud_cover REAL, humidity REAL, pressure REAL, visibility REAL,  ozone REAL, fio_forecast_id INTEGER REFERENCES fio_forecast(id) ON DELETE CASCADE, fio_day_info_id INTEGER REFERENCES fio_day_info(id) ON DELETE SET NULL);");
            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_day_info(id INTEGER PRIMARY KEY, sunrise INTEGER, sunset INTEGER, moon_phase REAL);");
        } catch (APSQLException& e) {
            db->RollbackTransaction();
            throw e;
        }

        db->EndTransaction();
    }
}

void APDarkSkyForcastIO::UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config) {
    if (db != NULL) {
        printf("Getting Forecast.io weather conditions...\n");
        char *response = _getForecast(config);
        if (response != NULL) {
            _parseJSONResponse(response, db, true);
        }
    }
}

char* APDarkSkyForcastIO::_getForecast(Json::Value& config) {
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

void APDarkSkyForcastIO::_parseJSONResponse(char *response, APSimpleSQL *db, bool live_condition) {
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
        // Get the city info
        Json::Value currently = json["currently"];
        if (currently != Json::Value::null && currently["time"] != Json::Value::null) {
            // Prep and store the main forecast object
            std::vector<APKeyValuePair*>* pairs = new std::vector<APKeyValuePair*>();
            // type
            APKeyValuePair* pair = new APKeyValuePair("type", "currently");
            pairs->push_back(pair);
            // time
            int64_t timeOfReport = currently["time"].asInt64();
            pair = new APKeyValuePair("time", timeOfReport);
            pairs->push_back(pair);
            // summary
            if (currently["summary"] != Json::Value::null) {
                pair = new APKeyValuePair("summary", currently["summary"].asString());
                pairs->push_back(pair);
            }
            // icon
            if (currently["icon"] != Json::Value::null) {
                pair = new APKeyValuePair("icon", currently["icon"].asString());
                pairs->push_back(pair);
            }
            int64_t forecast_id = db->DoInsert("fio_forecast", pairs);
            // Cleanup
            for (std::vector<APKeyValuePair*>::iterator it = pairs->begin() ; it != pairs->end(); ++it) {
                delete (*it);
            }
            delete pairs;


        }
    } catch (APSQLException& e) {
        db->RollbackTransaction();
        throw e;
    }

    db->EndTransaction();
}