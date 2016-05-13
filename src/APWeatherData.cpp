//
// Created by David Trotz on 5/9/16.
//

#include "APWeatherData.h"
#include <fstream>
#include <json/json.h>
#include <curl/curl.h>
#include <sstream>
#include <string.h>

APWeatherData::APWeatherData() {
    _db = NULL;
    _curlResponseChunk.memory = NULL;
    _curlResponseChunk.size = 0;
}

APWeatherData::~APWeatherData() {
    sqlite3_close(_db);
    free(_curlResponseChunk.memory);
}

bool APWeatherData::init(std::string configPath) {
    bool success = false;
    if (configPath.length() > 0) {
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

            rc = sqlite3_exec(_db, "CREATE INDEX IF NOT EXISTS owm_forecast_entry_dt ON owm_forecast_entry (dt);", NULL, 0, &zErrMsg);
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

static size_t
WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)userp;

    mem->memory = (char*)realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char* APWeatherData::Get5DayForecast() {
    CURL *curl;
    CURLcode res;
    if(_curlResponseChunk.memory != NULL) {
        free(_curlResponseChunk.memory);
    }
    _curlResponseChunk.memory = (char*)malloc(1);  /* will be grown as needed by the realloc above */
    _curlResponseChunk.size = 0;    /* no data at this point */

    curl = curl_easy_init();
    std::string lat = _config["latitude"].asString();
    std::string lon = _config["longitude"].asString();
    std::string apikey = _config["open_weather_map_apikey"].asString();
    if (curl != NULL) {
        char url[4096];
        snprintf(url, sizeof(url), "http://api.openweathermap.org/data/2.5/forecast?lat=%s&lon=%s&appid=%s", lat.c_str(), lon.c_str(), apikey.c_str());
        fprintf(stdout, "Trying: %s\n", url);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&_curlResponseChunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    return _curlResponseChunk.memory;
}

void APWeatherData::ParseAndStore5DayForecastResponse(char* response) {
    std::stringstream s;
    s << response;
    Json::Value json;
    s >> json;

    _beginTransation();

    std::string city_id = json["city"]["id"].asString();
    std::string city_name = json["city"]["name"].asString();
    std::string insertCity = "INSERT INTO owm_city (id, name) VALUES ('" + city_id + "', '" + city_name + "')";
    _doSQL(insertCity.c_str());

    _endTransation();
}

bool APWeatherData::_beginTransation() {
    return _doSQL("BEGIN TRANSACTION");
}

bool APWeatherData::_endTransation() {
    return _doSQL("COMMIT");
}

bool APWeatherData::_doSQL(const char* sql){
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
    int rc = sqlite3_step(stmt);                                                                    /* 3 */
    if (rc != SQLITE_DONE) {
        printf("ERROR stepping statement: %s\n", sqlite3_errmsg(_db));
    }

    sqlite3_finalize(stmt);
}