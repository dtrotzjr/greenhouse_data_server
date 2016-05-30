#include <iostream>
#include "APWeatherDataManager.h"
#include "APSimpleSQL.h"
#include "APGreenhouse.h"
#include <unistd.h>
#include <fstream>
#include <json/json.h>
#include <sstream>
#include "APException.h"

int main(int argc, char* argv[]) 
{
    if (argc == 2) {
        std::string configPath = argv[1];
	if (configPath.length() > 0) {
            Json::Value config;
            std::ifstream jsonFileStream;
            jsonFileStream.open(configPath);
            jsonFileStream >> config;

            std::string databaseFile = config["sqlite3_file"].asString();
            if (databaseFile.length() > 0) {
                APSimpleSQL *sqlDb = new APSimpleSQL(databaseFile);

                APWeatherDataManager* wm = new APWeatherDataManager(sqlDb, config);
                APGreenhouse* gh = new APGreenhouse(sqlDb, config);
                gh->InitializeSQLTables();
                while(true) {
                    try {
                        wm->GetLatestWeatherData();
                        gh->GetLatestSensorData();
                        sleep(60);
                    } catch (APException& e) {
                        break;
                    }
                }
            }
        }
    }
    std::cout << "Usage " + std::string(argv[1]) + " <path to config.json>" << std::endl;
    return -1;
}
