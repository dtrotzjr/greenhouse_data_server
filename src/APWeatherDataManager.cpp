//
// Created by David Trotz on 5/9/16.
//

#include "APWeatherDataManager.h"
#include "APOpenWeatherMap.h"
#include "APDarkSkyForcastIO.h"
#include "APException.h"
#include "APSimpleSQL.h"
#include <fstream>
#include <json/json.h>
#include <sstream>

APWeatherDataManager::APWeatherDataManager(std::string configPath) {
    if (configPath.length() > 0) {
        std::ifstream jsonFileStream;
        jsonFileStream.open(configPath);
        jsonFileStream >> _config;

        char *zErrMsg = 0;
        int rc;
        std::string databaseFile = _config["sqlite3_file"].asString();
        if (databaseFile.length() > 0) {
            _sqlDb = new APSimpleSQL(databaseFile);
            _sources = new std::vector<APWeatherSource*>();
            APOpenWeatherMap* openWeatherMap = new APOpenWeatherMap();
            openWeatherMap->InitializeSQLTables(_sqlDb);
            _sources->push_back(openWeatherMap);
        } else {
            throw APException("Missing database file in config file.");
        }

    } else {
        throw APException("Missing path to config.json file.");;
    }
}

APWeatherDataManager::~APWeatherDataManager() {
    for (std::vector<APWeatherSource*>::iterator it = _sources->begin() ; it != _sources->end(); ++it) {
        delete (*it);
    }
    delete _sources;
}

void APWeatherDataManager::GetLatestWeatherData() {
    for (std::vector<APWeatherSource*>::iterator it = _sources->begin() ; it != _sources->end(); ++it) {
        (*it)->UpdateWeatherInfo(_sqlDb, _config);
    }
}





