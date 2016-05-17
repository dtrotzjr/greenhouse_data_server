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
#include <time.h>
#include <sys/time.h>
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

            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_detail (id INTEGER PRIMARY KEY AUTOINCREMENT, dt INT, main_temp_k REAL, main_temp_min_k REAL, main_temp_max_k REAL, main_pressure REAL, main_humidity REAL, clouds_percent REAL, wind_speed_mps REAL, wind_deg REAL, rain_vol_last_3h REAL, snow_last_3h REAL, days_since_epoch INTEGER, live_condition INTEGER, owm_day_id INTEGER REFERENCES owm_day_info(id) ON DELETE SET NULL, owm_city_id INT REFERENCES owm_city(id) ON DELETE CASCADE);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_detail_dt_idx ON owm_detail (dt);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_detail_days_since_epoch_idx ON owm_detail (days_since_epoch);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_detail_to_weather_map (id INTEGER PRIMARY KEY AUTOINCREMENT, owm_detail_id INT REFERENCES owm_detail(id) ON DELETE SET NULL, owm_weather_id INT REFERENCES owm_weather(id) ON DELETE SET NULL);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_detail_to_weather_map_owm_detail_id_idx ON owm_detail_to_weather_map (owm_detail_id);");
            db->DoSQL( "CREATE INDEX IF NOT EXISTS owm_detail_to_weather_map_owm_weather_id_idx ON owm_detail_to_weather_map (owm_weather_id);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS owm_day_info(id INTEGER PRIMARY KEY, sunrise INTEGER, sunset INTEGER);");
        } catch (APSQLException& e) {
            db->RollbackTransaction();
            throw e;
        }

        db->EndTransaction();
    }
}

void APOpenWeatherMap::UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config) {
    if (db != NULL) {
        printf("Getting open weather map current conditions...\n");
        char* response = _getCurrentConditions(config);
        if (response != NULL) {
            _parseJSONResponse(response, db, true);
        }

        time_t now = time(NULL);
        int daysSinceEpoch = now / (60 * 60 * 24);
        char buff[4096];
        snprintf(buff, 4096,"SELECT id FROM owm_detail WHERE days_since_epoch = %d AND live_condition = 0 LIMIT 1", daysSinceEpoch);
        db->BeginSelect(buff);
        if (!db->StepSelect()) {
            printf("Getting open weather map forecast...\n");
            response = _getForecast(config);
            if (response != NULL) {
                _parseJSONResponse(response, db, false);
            }
        } else {
            printf("Skipping open weather map forecast since we already have it for today...\n");
        }
        db->EndSelect();

    } else {
        throw APException("Database object missing!\n");
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
        snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/weather?lat=%s&lon=%s&appid=%s", lat.c_str(), lon.c_str(), apikey.c_str());

        response = _jsonQueryObj->GetJSONResponseFromURL(url);
    }
    return response;
}

void APOpenWeatherMap::_parseJSONResponse(char *response, APSimpleSQL *db, bool live_condition) {
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
        time_t now = time(NULL);
        int daysSinceEpoch = now / (60 * 60 * 24);

        // Get the city info
        int64_t city_id;
        std::string city_name;
        bool missingCity = false;
        if (json["city"]["id"] != Json::Value::null) {
            // Forecast city id is in a city entry
            city_id = json["city"]["id"].asInt64();
        } else if (json["id"] != Json::Value::null) {
            // Current conditions city id is in main entry
            city_id = json["id"].asInt64();
        } else {
            city_id = 0;
        }
        if (json["city"]["name"] != Json::Value::null) {
            // Forecast city name is in a city entry
            city_name = json["city"]["name"].asString();
        } else if (json["id"] != Json::Value::null) {
            // Current conditions city name is in main entry
            city_name = json["name"].asString();
        } else {
            city_name = "Missing City Info";
        }

        if (!db->RowExists("owm_city", city_id)) {
            snprintf(buff, buffSize, "INSERT INTO owm_city (id, name) VALUES ('%lld', '%s');", city_id, city_name.c_str());
            db->DoSQL(buff);
        }

        // Get the data
        std::vector<Json::Value> res;
        if (json["list"] != Json::Value::null) {
            for (Json::Value& listItem : json["list"])
            {
                _parseWeatherInfo(listItem, db, live_condition, city_id, now, daysSinceEpoch);
            }
        } else {
            _parseWeatherInfo(json, db, live_condition, city_id, now, daysSinceEpoch);
        }

    } catch (APSQLException& e) {
        db->RollbackTransaction();
        throw e;
    }

    db->EndTransaction();
}

void APOpenWeatherMap::_parseWeatherInfo(Json::Value& json, APSimpleSQL *db, bool live_condition, int64_t city_id, time_t now, int daysSinceEpoch)
{
    int buffSize = 16384;
    char buff[buffSize];
    // Get the weather info first so we can create a weather entry for it if we need to then refer to it in the
    // forecast entry
    std::vector<int64_t > weather_conditions;
    for (const Json::Value& weatherItem : json["weather"]) {
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

    struct tm *today = localtime(&now);
    today->tm_hour = 0;
    today->tm_min = 0;
    today->tm_sec = 0;
    int64_t day_id = (int64_t)mktime(today);
    if(json["sys"]["sunrise"] != Json::Value::null && json["sys"]["sunset"] != Json::Value::null) {
        int64_t sunrise = json["sys"]["sunrise"].asInt64();
        int64_t sunset = json["sys"]["sunset"].asInt64();
        if(db->RowExists("owm_day_info", day_id)) {
            snprintf(buff, buffSize, "UPDATE owm_day_info SET sunrise = %lld, sunset = %lld WHERE id = %lld", sunrise, sunset, day_id);
            db->DoSQL(buff);
        } else {
            snprintf(buff, buffSize, "INSERT INTO owm_day_info (id, sunrise ,sunset) VALUES (%lld, %lld, %lld)", day_id, sunrise, sunset);
            db->DoSQL(buff);
        }
    }

    // Weather details
    int64_t dt = json["dt"].asInt64();
    // Main
    float main_temp_k = json["main"]["temp"].asFloat();
    float main_temp_min_k = json["main"]["temp_min"].asFloat();
    float main_temp_max_k = json["main"]["temp_max"].asFloat();
    float main_pressure = json["main"]["pressure"].asFloat();
    int main_humidity = json["main"]["humidity"].asInt();
    // Clouds
    int clouds_all = json["clouds"]["all"].asInt();
    // Wind
    float wind_speed = json["wind"]["speed"].asFloat();
    float wind_deg = json["wind"]["deg"].asFloat();
    // Rain
    float rain_3h = 0.0f;
    if (json["rain"]["3h"] != Json::Value::null) {
        rain_3h = json["rain"]["3h"].asFloat();
    }
    // Snow
    float snow_3h = 0.0f;
    if (json["snow"]["3h"] != Json::Value::null) {
        snow_3h = json["snow"]["3h"].asFloat();
    }

    snprintf(buff, buffSize, "INSERT INTO owm_detail (dt, main_temp_k, main_temp_min_k, main_temp_max_k, main_pressure, main_humidity, clouds_percent, wind_speed_mps, wind_deg, rain_vol_last_3h, snow_last_3h, days_since_epoch, live_condition, owm_day_id, owm_city_id) VALUES ('%lld', '%f', '%f', '%f', '%f', '%d', '%d', '%f', '%f', '%f', '%f', '%d', '%d', '%lld', '%lld');", dt, main_temp_k, main_temp_min_k, main_temp_max_k, main_pressure, main_humidity, clouds_all, wind_speed, wind_deg, rain_3h, snow_3h, daysSinceEpoch, live_condition, day_id, city_id);
    int64_t lastRowId = db->DoInsert(buff);
    if (lastRowId >= 0) {
        for (std::vector<int64_t>::iterator it = weather_conditions.begin() ; it != weather_conditions.end(); ++it) {
            snprintf(buff, buffSize, "INSERT INTO owm_detail_to_weather_map (owm_detail_id, owm_weather_id) VALUES (%lld, %lld)", lastRowId, (*it));
            db->DoSQL(buff);
        }
    }
}