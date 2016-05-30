//
// Created by David Trotz on 5/9/16.
//

#include "APWeatherDataManager.h"
#include "APOpenWeatherMap.h"
#include "APDarkSkyForecastIO.h"
#include "APException.h"
#include "APSimpleSQL.h"


APWeatherDataManager::APWeatherDataManager(Json::Value config) {
    if (config != Json::Value::null) {
        _config = config;
        std::string databaseFile = config["sqlite3_file"].asString();
        _sqlDb = new APSimpleSQL(databaseFile);

        _sources = new std::vector<APWeatherSource*>();
        APOpenWeatherMap* openWeatherMap = new APOpenWeatherMap();
        openWeatherMap->InitializeSQLTables(_sqlDb);
        _sources->push_back(openWeatherMap);

        APDarkSkyForecastIO* forecastIO = new APDarkSkyForecastIO();
        forecastIO->InitializeSQLTables(_sqlDb);
        _sources->push_back(forecastIO);
    } else {
        throw APException("Invalid constructor arguments.");;
    }
}

APWeatherDataManager::~APWeatherDataManager() {
    for (std::vector<APWeatherSource*>::iterator it = _sources->begin() ; it != _sources->end(); ++it) {
        delete (*it);
    }
    delete _sources;
    delete _sqlDb;
}

void APWeatherDataManager::GetLatestWeatherData() {
    for (std::vector<APWeatherSource*>::iterator it = _sources->begin() ; it != _sources->end(); ++it) {
        (*it)->UpdateWeatherInfo(_sqlDb, _config);
    }
}

