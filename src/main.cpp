#include <iostream>
#include "APWeatherDataManager.h"
#include "APSimpleSQL.h"
#include "APGreenhouse.h"
#include <unistd.h>
#include <fstream>
#include <json/json.h>
#include <sstream>
#include <pthread.h>
#include "APException.h"
#include "APImageTransferAgent.h"


void *transfer_thread(void *x_void_ptr)
{
    APImageTransferAgent* ita = static_cast<APImageTransferAgent*>(x_void_ptr);
    ita->Run();
    return NULL;
}

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

                APImageTransferAgent* ita = new APImageTransferAgent(config);
                pthread_t inc_x_thread;
                if(pthread_create(&inc_x_thread, NULL, transfer_thread, ita)) {
                    fprintf(stderr, "Error creating image transfer thread.\n");
                    return 1;

                }
                APWeatherDataManager* wm = new APWeatherDataManager(config);
                APGreenhouse* gh = new APGreenhouse(config);
                gh->InitializeSQLTables();
                while(true) {
                    try {
                        time_t start = time(NULL);
                        wm->GetLatestWeatherData();
                        gh->GetLatestSensorData();
                        time_t delta = time(NULL) - start;
                        int waitFor = 60;
                        if  (delta > 60)
                            waitFor = 0;
                        else
                            waitFor = 60 - delta;
                        sleep(waitFor);
                    } catch (APException& e) {
                        fprintf(stderr, "Exception Caught: %s", e.what());
                        break;
                    }
                }
            }
        }
    }
    std::cout << "Usage " + std::string(argv[1]) + " <path to config.json>" << std::endl;
    return -1;
}
