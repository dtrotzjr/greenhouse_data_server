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

bool APWeatherData::init(std::string* dbPath) {
    bool failed = false;
    char *zErrMsg = 0;
    int rc;

    rc = sqlite3_open(dbPath->c_str(), &_db);
    if(rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
        sqlite3_close(_db);
        failed = true;
    }
//        rc = sqlite3_exec(_db, argv[2], callback, 0, &zErrMsg);
//        if( rc!=SQLITE_OK ){
//            fprintf(stderr, "SQL error: %s\n", zErrMsg);
//            sqlite3_free(zErrMsg);
//        }

    return !failed;
}