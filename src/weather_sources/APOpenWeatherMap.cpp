//
// Created by David Trotz on 5/15/16.
//

#include "APOpenWeatherMap.h"
#include "APSimpleSQL.h"
#include "APSimpleJSONQuery.h"
#include "APException.h"
#include "APSQLException.h"
#include <string>
#include <sstream>
#include <json/json.h>

APOpenWeatherMap::APOpenWeatherMap() {
    _jsonQueryObj = new APSimpleJSONQuery();
}

APOpenWeatherMap::~APOpenWeatherMap() {
    delete _jsonQueryObj;
}

void APOpenWeatherMap::InitializeSQLTables(APSimpleSQL* db) {
    // Create the needed tables
    if (db != NULL) {
        db->BeginTransaction();
        try{
            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_city (id INT PRIMARY KEY, name TEXT DEFAULT \"Unnamed Location\");");
            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_weather (id INT PRIMARY KEY, main TEXT, description TEXT, icon TEXT);");
            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_forecast_entry (id INT PRIMARY KEY, dt INT, main_temp_k REAL, main_temp_min_k REAL, main_temp_max_k REAL, main_pressure REAL, main_sea_level REAL, main_ground_level REAL, main_humidity REAL, temp_kf REAL, clouds_percent REAL, wind_speed_mps REAL, wind_deg REAL, rain_vol_last_3h REAL, snow_last_3h REAL, dt_txt TEXT, owm_weather_id INT REFERENCES owm_weather(id) ON DELETE CASCADE, owm_city_id INT REFERENCES owm_city(id) ON DELETE CASCADE);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_forecast_entry_dt ON owm_forecast_entry (dt);");
        } catch (APSQLException& e) {
            db->RollbackTransaction();
            throw e;
        }

        db->EndTransaction();
    }
}

void APOpenWeatherMap::UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config) {
    if (db != NULL) {
        char* response = _getForecast(config);
        if (response != NULL) {
            fprintf(stdout, response);
            _parseResponse(response, db);
        }
    } else {
        throw APException("Database object missing!");
    }
}

char* APOpenWeatherMap::_getForecast(Json::Value& config) {
    char* response = NULL;
    std::string lat = config["latitude"].asString();
    std::string lon = config["longitude"].asString();
    std::string apikey = config["open_weather_map_apikey"].asString();
    if (lat.length() > 0 && lon.length() > 0 && apikey.length() > 0) {
        char url[2048];
        snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/forecast?lat=%s&lon=%s&appid=%s", lat.c_str(), lon.c_str(), apikey.c_str());

        response = _jsonQueryObj->GetJSONResponseFromURL(url);
    }
    return response;
}

void APOpenWeatherMap::_parseResponse(char* response, APSimpleSQL* db) {
    // Prep the JSON object...
    std::stringstream s;
    s << response;
    Json::Value json;
    s >> json;
    // Store the values in the database
    db->BeginTransaction();
    try{
        std::string city_id = json["city"]["id"].asString();
        std::string city_name = json["city"]["name"].asString();
        std::string insertCitySQL = "INSERT INTO owm_city (id, name) VALUES ('" + city_id + "', '" + city_name + "')";
        db->DoSQL(insertCitySQL.c_str());

    } catch (APSQLException& e) {
        db->RollbackTransaction();
        throw e;
    }

    db->EndTransaction();
}