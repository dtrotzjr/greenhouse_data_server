//
// Created by David Trotz on 5/29/16.
//

#include "APGreenhouse.h"
#include "APSimpleJSONQuery.h"
#include "APSimpleSQL.h"
#include "APSQLException.h"

APGreenhouse::APGreenhouse(APSimpleSQL* sqlDb, Json::Value& config) {
    _jsonQueryObj = new APSimpleJSONQuery();
    _sqlDb = sqlDb;
    _config = config;
}

APGreenhouse::~APGreenhouse() {
    delete _jsonQueryObj;
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

}
