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

            pairs = new std::vector<APKeyValuePair*>();
            // nearestStormDistance
            if (currently["nearestStormDistance"] != Json::Value::null) {
                pair = new APKeyValuePair("nearest_storm_distance", currently["nearestStormDistance"].asDouble());
                pairs->push_back(pair);
            }
            // nearestStormBearing
            if (currently["nearestStormBearing"] != Json::Value::null) {
                pair = new APKeyValuePair("nearest_storm_bearing", currently["nearestStormBearing"].asDouble());
                pairs->push_back(pair);
            }
            // precipIntensity
            if (currently["precipIntensity"] != Json::Value::null) {
                pair = new APKeyValuePair("precip_intensity", currently["precipIntensity"].asDouble());
                pairs->push_back(pair);
            }
            // precipIntensityMax
            if (currently["precipIntensityMax"] != Json::Value::null) {
                pair = new APKeyValuePair("precip_intensity_max", currently["precipIntensityMax"].asDouble());
                pairs->push_back(pair);
            }
            // precipIntensityMaxTime
            if (currently["precipIntensityMaxTime"] != Json::Value::null) {
                pair = new APKeyValuePair("precip_intensity_max_time", currently["precipIntensityMaxTime"].asInt64());
                pairs->push_back(pair);
            }
            // precipProbability
            if (currently["precipProbability"] != Json::Value::null) {
                pair = new APKeyValuePair("precip_probability", currently["precipProbability"].asDouble());
                pairs->push_back(pair);
            }
            // precipType
            if (currently["precipType"] != Json::Value::null) {
                pair = new APKeyValuePair("precip_type", currently["precipType"].asString());
                pairs->push_back(pair);
            }
            // precipAccumulation
            if (currently["precipAccumulation"] != Json::Value::null) {
                pair = new APKeyValuePair("precip_accumulation", currently["precipAccumulation"].asDouble());
                pairs->push_back(pair);
            }
            // temperature
            if (currently["temperature"] != Json::Value::null) {
                pair = new APKeyValuePair("temperature", currently["temperature"].asDouble());
                pairs->push_back(pair);
            }
            // temperatureMin
            if (currently["temperatureMin"] != Json::Value::null) {
                pair = new APKeyValuePair("temperature_min", currently["temperatureMin"].asDouble());
                pairs->push_back(pair);
            }
            // temperatureMinTime
            if (currently["temperatureMinTime"] != Json::Value::null) {
                pair = new APKeyValuePair("temperature_min_time", currently["temperatureMinTime"].asInt64());
                pairs->push_back(pair);
            }
            // temperatureMax
            if (currently["temperatureMax"] != Json::Value::null) {
                pair = new APKeyValuePair("temperature_max", currently["temperatureMax"].asDouble());
                pairs->push_back(pair);
            }
            // temperatureMaxTime
            if (currently["temperatureMaxTime"] != Json::Value::null) {
                pair = new APKeyValuePair("temperature_max_time", currently["temperatureMaxTime"].asInt64());
                pairs->push_back(pair);
            }
            // apparentTemperature
            if (currently["apparentTemperature"] != Json::Value::null) {
                pair = new APKeyValuePair("apparent_temperature", currently["apparentTemperature"].asDouble());
                pairs->push_back(pair);
            }
            // apparentTemperatureMin
            if (currently["apparentTemperatureMin"] != Json::Value::null) {
                pair = new APKeyValuePair("apparent_temperature_min", currently["apparentTemperatureMin"].asDouble());
                pairs->push_back(pair);
            }
            // apparentTemperatureMinTime
            if (currently["apparentTemperatureMinTime"] != Json::Value::null) {
                pair = new APKeyValuePair("apparent_temperature_min_time", currently["apparentTemperatureMinTime"].asInt64());
                pairs->push_back(pair);
            }
            // apparentTemperatureMax
            if (currently["apparentTemperatureMax"] != Json::Value::null) {
                pair = new APKeyValuePair("apparent_temperature_max", currently["apparentTemperatureMax"].asDouble());
                pairs->push_back(pair);
            }
            // apparentTemperatureMaxTime
            if (currently["apparentTemperatureMaxTime"] != Json::Value::null) {
                pair = new APKeyValuePair("apparent_temperature_max_time", currently["apparentTemperatureMaxTime"].asInt64());
                pairs->push_back(pair);
            }
            // dewPoint
            if (currently["dewPoint"] != Json::Value::null) {
                pair = new APKeyValuePair("dew_point", currently["dewPoint"].asDouble());
                pairs->push_back(pair);
            }
            // windSpeed
            if (currently["windSpeed"] != Json::Value::null) {
                pair = new APKeyValuePair("wind_speed", currently["windSpeed"].asDouble());
                pairs->push_back(pair);
            }
            // windBearing
            if (currently["windBearing"] != Json::Value::null) {
                pair = new APKeyValuePair("wind_bearing", currently["windBearing"].asDouble());
                pairs->push_back(pair);
            }
            // cloudCover
            if (currently["cloudCover"] != Json::Value::null) {
                pair = new APKeyValuePair("cloud_cover", currently["cloudCover"].asDouble());
                pairs->push_back(pair);
            }
            // humidity
            if (currently["humidity"] != Json::Value::null) {
                pair = new APKeyValuePair("humidity", currently["humidity"].asDouble());
                pairs->push_back(pair);
            }
            // pressure
            if (currently["pressure"] != Json::Value::null) {
                pair = new APKeyValuePair("pressure", currently["pressure"].asDouble());
                pairs->push_back(pair);
            }
            // visibility
            if (currently["visibility"] != Json::Value::null) {
                pair = new APKeyValuePair("visibility", currently["visibility"].asDouble());
                pairs->push_back(pair);
            }
            // ozone
            if (currently["ozone"] != Json::Value::null) {
                pair = new APKeyValuePair("ozone", currently["ozone"].asDouble());
                pairs->push_back(pair);
            }
            pair = new APKeyValuePair("fio_forecast_id", forecast_id);
            pairs->push_back(pair);

            int64_t data_id = db->DoInsert("fio_data", pairs);
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