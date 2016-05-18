//
// Created by David Trotz on 5/15/16.
//
#include <json/json.h>
#include "APDarkSkyForcastIO.h"
#include "APSimpleSQL.h"

void APDarkSkyForcastIO::InitializeSQLTables(APSimpleSQL* db) {
    // Create the needed tables
    if (db != NULL) {
        db->BeginTransaction();
        try{
            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_forecast (id INTEGER PRIMARY KEY AUTOINCREMENT, type TEXT, summary TEXT, icon TEXT);");
            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_data (id INTEGER PRIMARY KEY AUTOINCREMENT, time INTEGER, summary TEXT, icon TEXT, nearest_storm_distance INTEGER, nearest_storm_bearing INTEGER, precip_intensity INTEGER, precip_probability INTEGER, temperature REAL, apparent_temperature REAL, dew_point REAL, humidity REAL, wind_speed REAL, wind_bearing REAL, visibility REAL, cloud_cover REAL, pressure REAL, ozone REAL, fio_forecast_id INTEGER REFERENCES fio_forecast(id) ON DELETE CASCADE);");
            db->DoSQL("CREATE TABLE IF NOT EXISTS fio_day_info(id INTEGER PRIMARY KEY, sunrise INTEGER, sunset INTEGER, moon_phase REAL);");
        } catch (APSQLException& e) {
            db->RollbackTransaction();
            throw e;
        }

        db->EndTransaction();
    }
}

void APDarkSkyForcastIO::UpdateWeatherInfo(APSimpleSQL* db, Json::Value& config) {
}

char* APDarkSkyForcastIO::_getForecast(Json::Value& config) {
    char* response = NULL;
    std::string lat = config["latitude"].asString();
    std::string lon = config["longitude"].asString();
    std::string apikey = config["open_weather_map_apikey"].asString();
    if (lat.length() > 0 && lon.length() > 0 && apikey.length() > 0) {
        char url[2048];
        snprintf(url, sizeof(url), "https://api.forecast.io/forecast/%s/%s,%s", apikey.c_str(), lat.c_str(), lon.c_str());

        response = _jsonQueryObj->GetJSONResponseFromURL(url);
    }
    return response;
}

void APDarkSkyForcastIO::_parseJSONResponse(char *response, APSimpleSQL *db, bool live_condition) {

}