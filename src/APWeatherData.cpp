//
// Created by David Trotz on 5/9/16.
//

#include "APWeatherData.h"



APWeatherData::APWeatherData() {
    _db = NULL;
}

APWeatherData::~APWeatherData() {
    sqlite3_close(_db);
}

bool APWeatherData::init(std::string dbPath) {
    bool failed = false;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(dbPath.c_str(), &_db);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
        sqlite3_close(_db);
        failed = true;
    }
    // Set the database encoding to UTF-8 (we don't care if it fails as this will only succeed the very first time
    // the database is created.
    rc = sqlite3_exec(_db, "PRAGMA encoding = \"UTF-8\";", NULL, 0, &zErrMsg);

    // Create the needed tables
    rc = sqlite3_exec(_db, "CREATE TABLE IF NOT EXISTS owm_cities (id INT PRIMARY KEY, name TEXT DEFAULT \"Unnamed Location\") ;", NULL, 0, &zErrMsg);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", zErrMsg);
        sqlite3_free(zErrMsg);
    }




    return !failed;
}
