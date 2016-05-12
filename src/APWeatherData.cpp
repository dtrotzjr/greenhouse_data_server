//
// Created by David Trotz on 5/9/16.
//

#include "APWeatherData.h"
#include <fstream>
#include <json/json.h>

APWeatherData::APWeatherData() {
    _db = NULL;
}

APWeatherData::~APWeatherData() {
    sqlite3_close(_db);
}

bool APWeatherData::init(std::string configPath) {
    bool success = false;
    if (configPath.length() > 0) {
        fprintf(stdout, "Got here. 2\n");
        if (_db != NULL)
            sqlite3_close(_db);

        std::ifstream jsonFileStream;
        jsonFileStream.open(configPath);
        jsonFileStream >> _config;

        char *zErrMsg = 0;
        int rc;
        //,root["open_weather_map_apikey"].asString()
        std::string sqliteFile = _config["sqlite3_file"].asString();
        if (sqliteFile.length() > 0) {
            rc = sqlite3_open(sqliteFile.c_str(), &_db);
            if(rc != SQLITE_OK) {
                fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
                sqlite3_close(_db);
            }
            // Set the database encoding to UTF-8 (we don't care if it fails as this will only succeed the very first time
            // the database is created.
            rc = sqlite3_exec(_db, "PRAGMA encoding = \"UTF-8\";", NULL, 0, &zErrMsg);

            // Create the needed tables
            rc = sqlite3_exec(_db, "CREATE TABLE IF NOT EXISTS owm_city (id INT PRIMARY KEY, name TEXT DEFAULT \"Unnamed Location\");", NULL, 0, &zErrMsg);
            if(rc != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }

            rc = sqlite3_exec(_db, "CREATE TABLE IF NOT EXISTS owm_weather (id INT PRIMARY KEY, main TEXT, description TEXT, icon TEXT);", NULL, 0, &zErrMsg);
            if(rc != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }


            rc = sqlite3_exec(_db, "CREATE TABLE IF NOT EXISTS owm_forecast_entry (id INT PRIMARY KEY, dt INT, main_temp_k REAL, main_temp_min_k REAL, main_temp_max_k REAL, main_pressure REAL, main_sea_level REAL, main_ground_level REAL, main_humidity REAL, temp_kf REAL, clouds_percent REAL, wind_speed_mps REAL, wind_deg REAL, rain_vol_last_3h REAL, snow_last_3h REAL, dt_txt TEXT, owm_weather_id INT REFERENCES owm_weather(id) ON DELETE CASCADE, owm_city_id INT REFERENCES owm_city(id) ON DELETE CASCADE);", NULL, 0, &zErrMsg);
            if(rc != SQLITE_OK) {
                fprintf(stderr, "SQL error: %s\n", zErrMsg);
                sqlite3_free(zErrMsg);
            }

            success = true;
        } else {
            fprintf(stderr, "ERROR: Missing sqlite_file entry in config.json");
        }
    } else {
        fprintf(stderr, "ERROR: Invalid config.json file path.");
    }
    return success;
}
