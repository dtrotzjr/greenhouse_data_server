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
#include <vector>
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
            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_city (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT DEFAULT \"Unnamed Location\");");

            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_weather (id INTEGER PRIMARY KEY AUTOINCREMENT, main TEXT, description TEXT, icon TEXT);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_forecast_entry (id INTEGER PRIMARY KEY AUTOINCREMENT, dt INT, main_temp_k REAL, main_temp_min_k REAL, main_temp_max_k REAL, main_pressure REAL, main_humidity REAL, clouds_percent REAL, wind_speed_mps REAL, wind_deg REAL, rain_vol_last_3h REAL, snow_last_3h REAL, days_since_epoch INTEGER, owm_city_id INT REFERENCES owm_city(id) ON DELETE CASCADE);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_forecast_entry_dt_idx ON owm_forecast_entry (dt);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_forecast_entry_days_since_epoch_idx ON owm_forecast_entry (days_since_epoch);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_forecast_entry_weather (id INTEGER PRIMARY KEY AUTOINCREMENT, owm_forecast_entry_id INT REFERENCES owm_forecast_entry(id) ON DELETE SET NULL, owm_weather_id INT REFERENCES owm_weather(id) ON DELETE SET NULL);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_forecast_entry_weather_owm_forecast_entry_id_idx ON owm_forecast_entry_weather (owm_forecast_entry_id);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_forecast_entry_weather_owm_weather_id_idx ON owm_forecast_entry_weather (owm_weather_id);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_current_conditions (id INTEGER PRIMARY KEY AUTOINCREMENT, dt INT, main_temp_k REAL, main_temp_min_k REAL, main_temp_max_k REAL, main_pressure REAL, main_humidity REAL, clouds_percent REAL, wind_speed_mps REAL, wind_deg REAL, rain_vol_last_3h REAL, snow_last_3h REAL, days_since_epoch INTEGER, owm_city_id INT REFERENCES owm_city(id) ON DELETE CASCADE);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_current_conditions_dt_idx ON owm_current_conditions(dt);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_current_conditions_days_since_epoch_idx ON owm_current_conditions(days_since_epoch);");
#error Stopped here - need to add the mapping table info
            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_current_conditions_weather (id INTEGER PRIMARY KEY AUTOINCREMENT, owm_current_conditions_id INT REFERENCES owm_current_conditions(id) ON DELETE SET NULL, owm_weather_id INT REFERENCES owm_weather(id) ON DELETE SET NULL);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_weather_owm_forecast_entry_id_idx ON owm_forecast_entry_weather (owm_forecast_entry_id);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_forecast_entry_weather_owm_weather_id_idx ON owm_forecast_entry_weather (owm_weather_id);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS day_info(id INTEGER PRIMARY KEY, sunrise INTEGER, sunset INTEGER);");
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
            _parseForecastResponse(response, db);
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

char* APOpenWeatherMap::_getCurrentConditions(Json::Value& config) {
    char* response = NULL;
    std::string lat = config["latitude"].asString();
    std::string lon = config["longitude"].asString();
    std::string apikey = config["open_weather_map_apikey"].asString();
    if (lat.length() > 0 && lon.length() > 0 && apikey.length() > 0) {
        char url[2048];
        snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/weather??lat=%s&lon=%s&appid=%s", lat.c_str(), lon.c_str(), apikey.c_str());

        response = _jsonQueryObj->GetJSONResponseFromURL(url);
    }
    return response;
}

void APOpenWeatherMap::_parseForecastResponse(char *response, APSimpleSQL *db) {
    int buffSize = 16384;
    char buff[buffSize];
    // Prep the JSON object...
    std::stringstream s;
    s << response;
    Json::Value json;
    s >> json;
    // Store the values in the database
    db->BeginTransaction();
    try{
        time_t seconds = time(NULL);
        int daysSinceEpoch = seconds/(60*60*24);

        // Get the city info
        int64_t city_id = json["city"]["id"].asInt64();
        std::string city_name = json["city"]["name"].asString();
        if (!db->RowExists("owm_city", city_id)) {
            snprintf(buff, buffSize, "INSERT INTO owm_city (id, name) VALUES ('%lld', '%s');", city_id, city_name.c_str());
            db->DoSQL(buff);
        }

        // Get the data
        std::vector<Json::Value> res;
        for (const Json::Value& listItem : json["list"])
        {
            // Get the weather info first so we can create a weather entry for it if we need to then refer to it in the
            // forecast entry
            std::vector<int64_t > weather_conditions;
            for (const Json::Value& weatherItem : listItem["weather"]) {
                uint64_t weather_id = weatherItem["id"].asInt64();
                weather_conditions.push_back(weather_id);
                std::string weather_main = weatherItem["main"].asString();
                std::string weather_description = weatherItem["description"].asString();
                std::string weather_icon = weatherItem["icon"].asString();
                if (!db->RowExists("owm_weather", weather_id)) {
                    snprintf(buff, buffSize, "INSERT INTO owm_weather (id, main, description, icon) VALUES ('%lld', '%s', '%s', '%s');", weather_id, weather_main.c_str(), weather_description.c_str(), weather_icon.c_str());
                    db->DoSQL(buff);
                }
            }



            // Forecast entry
            int64_t dt = listItem["dt"].asInt64();
            // Main
            float main_temp_k = listItem["main"]["temp"].asFloat();
            float main_temp_min_k = listItem["main"]["temp_min"].asFloat();
            float main_temp_max_k = listItem["main"]["temp_max"].asFloat();
            float main_pressure = listItem["main"]["pressure"].asFloat();
            int main_humidity = listItem["main"]["humidity"].asInt();
            // Clouds
            int clouds_all = listItem["clouds"]["all"].asInt();
            // Wind
            float wind_speed = listItem["wind"]["speed"].asFloat();
            float wind_deg = listItem["wind"]["deg"].asFloat();
            // Rain
            float rain_3h = 0.0f;
            if (listItem["rain"]["3h"] != Json::Value::null) {
                rain_3h = listItem["rain"]["3h"].asFloat();
            }
            // Snow
            float snow_3h = 0.0f;
            if (listItem["snow"]["3h"] != Json::Value::null) {
                snow_3h = listItem["snow"]["3h"].asFloat();
            }
            snprintf(buff, buffSize, "INSERT INTO owm_forecast_entry (dt, main_temp_k, main_temp_min_k, main_temp_max_k, main_pressure, main_humidity, clouds_percent, wind_speed_mps, wind_deg, rain_vol_last_3h, snow_last_3h, days_since_epoch, owm_city_id) VALUES ('%lld', '%f', '%f', '%f', '%f', '%d', '%d', '%f', '%f', '%f', '%f', '%d', '%lld');", dt, main_temp_k, main_temp_min_k, main_temp_max_k, main_pressure, main_humidity, clouds_all, wind_speed, wind_deg, rain_3h, snow_3h, daysSinceEpoch, city_id);
            int64_t lastRowId = db->DoInsert(buff);
            if (lastRowId >= 0) {
                for (std::vector<int64_t>::iterator it = weather_conditions.begin() ; it != weather_conditions.end(); ++it) {
                    snprintf(buff, buffSize, "INSERT INTO owm_forecast_entry_weather (owm_forecast_entry_id, owm_weather_id) VALUES (%lld, %lld)", lastRowId, (*it));
                    db->DoSQL(buff);
                }
            }
        }
    } catch (APSQLException& e) {
        db->RollbackTransaction();
        throw e;
    }

    db->EndTransaction();
}

void APOpenWeatherMap::_parseCurrentConditions(char *response, APSimpleSQL *db) {
    int buffSize = 16384;
    char buff[buffSize];
    // Prep the JSON object...
    std::stringstream s;
    s << response;
    Json::Value json;
    s >> json;
    // Store the values in the database
    db->BeginTransaction();
    try{
        time_t seconds = time(NULL);
        int daysSinceEpoch = seconds/(60*60*24);

        // Get the city info
        int64_t city_id = json["id"].asInt64();
        std::string city_name = json["name"].asString();
        if (!db->RowExists("owm_city", city_id)) {
            snprintf(buff, buffSize, "INSERT INTO owm_city (id, name) VALUES ('%lld', '%s');", city_id, city_name.c_str());
            db->DoSQL(buff);
        }

        // Get the data
        std::vector<Json::Value> res;
        for (const Json::Value& listItem : json["list"])
        {
            // Get the weather info first so we can create a weather entry for it if we need to then refer to it in the
            // forecast entry
            std::vector<int64_t > weather_conditions;
            for (const Json::Value& weatherItem : listItem["weather"]) {
                uint64_t weather_id = weatherItem["id"].asInt64();
                weather_conditions.push_back(weather_id);
                std::string weather_main = weatherItem["main"].asString();
                std::string weather_description = weatherItem["description"].asString();
                std::string weather_icon = weatherItem["icon"].asString();
                if (!db->RowExists("owm_weather", weather_id)) {
                    snprintf(buff, buffSize, "INSERT INTO owm_weather (id, main, description, icon) VALUES ('%lld', '%s', '%s', '%s');", weather_id, weather_main.c_str(), weather_description.c_str(), weather_icon.c_str());
                    db->DoSQL(buff);
                }
            }



            // Forecast entry
            int64_t dt = listItem["dt"].asInt64();
            // Main
            float main_temp_k = listItem["main"]["temp"].asFloat();
            float main_temp_min_k = listItem["main"]["temp_min"].asFloat();
            float main_temp_max_k = listItem["main"]["temp_max"].asFloat();
            float main_pressure = listItem["main"]["pressure"].asFloat();
            int main_humidity = listItem["main"]["humidity"].asInt();
            // Clouds
            int clouds_all = listItem["clouds"]["all"].asInt();
            // Wind
            float wind_speed = listItem["wind"]["speed"].asFloat();
            float wind_deg = listItem["wind"]["deg"].asFloat();
            // Rain
            float rain_3h = 0.0f;
            if (listItem["rain"]["3h"] != Json::Value::null) {
                rain_3h = listItem["rain"]["3h"].asFloat();
            }
            // Snow
            float snow_3h = 0.0f;
            if (listItem["snow"]["3h"] != Json::Value::null) {
                snow_3h = listItem["snow"]["3h"].asFloat();
            }
            snprintf(buff, buffSize, "INSERT INTO owm_current_conditions (dt, main_temp_k, main_temp_min_k, main_temp_max_k, main_pressure, main_humidity, clouds_percent, wind_speed_mps, wind_deg, rain_vol_last_3h, snow_last_3h, days_since_epoch, owm_city_id) VALUES ('%lld', '%f', '%f', '%f', '%f', '%d', '%d', '%f', '%f', '%f', '%f', '%d', '%lld');", dt, main_temp_k, main_temp_min_k, main_temp_max_k, main_pressure, main_humidity, clouds_all, wind_speed, wind_deg, rain_3h, snow_3h, daysSinceEpoch, city_id);
            int64_t lastRowId = db->DoInsert(buff);
            if (lastRowId >= 0) {
                for (std::vector<int64_t>::iterator it = weather_conditions.begin() ; it != weather_conditions.end(); ++it) {
                    snprintf(buff, buffSize, "INSERT INTO owm_forecast_entry_weather (owm_forecast_entry_id, owm_weather_id) VALUES (%lld, %lld)", lastRowId, (*it));
                    db->DoSQL(buff);
                }
            }
        }
    } catch (APSQLException& e) {
        db->RollbackTransaction();
        throw e;
    }

    db->EndTransaction();
}