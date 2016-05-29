//
// Created by David Trotz on 5/29/16.
//

#include "APGreenhouse.h"
#include "APSimpleJSONQuery.h"

APGreenhouse::APGreenhouse() {
    _jsonQueryObj = new APSimpleJSONQuery();
}

virtual APGreenhouse::~APGreenhouse() {
    delete _jsonQueryObj;
}

void APGreenhouse::InitializeSQLTables(APSimpleSQL* db) {
    // Create the needed tables
    if (db != NULL) {
        db->BeginTransaction();
        try{
            db->DoSQL("CREATE TABLE IF NOT EXISTS gh_data_points (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, timestamp integer, synchronized integer DEFAULT 0);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS index_gh_data_points_on_timestamp ON gh_data_points (timestamp);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS gh_sensor_data (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, sensor_id integer, temperature float, humidity float, gh_data_point_id integer);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS index_gh_sensor_data_on_gh_data_point_id ON gh_sensor_data (gh_data_point_id);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS gh_system_data (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, soc_temperature float, wlan0_link_quality float, wlan0_signal_level integer, storage_total_size integer,storage_used integer, storage_avail integer, gh_data_point_id integer);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS index_gh_system_data_on_gh_data_point_id ON gh_system_data(gh_data_point_id);");

            db->DoSQL("CREATE TABLE IF NOT EXISTS gh_image_data (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, filename text, gh_data_point_id integer);");
            db->DoSQL("CREATE INDEX IF NOT EXISTS index_gh_image_data_on_gh_data_point_id ON gh_image_data(gh_data_point_id);");
        } catch (APSQLException& e) {
            db->RollbackTransaction();
            throw e;
        }

        db->EndTransaction();
    }
}

void APGreenhouse::UpdateGreenhouseSensorData(APSimpleSQL* db, Json::Value& config) {

}