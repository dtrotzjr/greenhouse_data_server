#include <iostream>
#include "APWeatherDataManager.h"
#include "APSimpleSQL.h"
#include "APGreenhouse.h"
#include <unistd.h>
#include <fstream>
#include <string.h>
#include <json/json.h>
#include <sstream>
#include <pthread.h>
#include "APException.h"
#include "APImageTransferAgent.h"
#include <ctime>

APImageTransferAgent *startImageTransferOnAThread(Json::Value& config);

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

                APImageTransferAgent* ita = startImageTransferOnAThread(config);

                APWeatherDataManager* wm = new APWeatherDataManager(config);
                APGreenhouse* gh = new APGreenhouse(config);
                gh->InitializeSQLTables();
                int sleepLen = 0;
                while(true) {
                    // Print a header...
                    time_t rawtime_now = time(NULL);
                    time( &rawtime_now);
                    tm* time_now = localtime(&rawtime_now);
                    int timeBufLen = 64;
                    char timeBuf[timeBufLen];
                    strftime(timeBuf, timeBufLen, "%F %T", time_now);
                    int timeStrLen = (int)strlen(timeBuf);
                    fprintf(stdout, "+--------------------------------------------------------------------------------------------------+\n");
                    fprintf(stdout, "| %s", timeBuf);
                    if (timeStrLen < 77) {
                        for (int i = 0; i < (77 - timeStrLen); i++) {
                            fprintf(stdout, " ");
                        }
                        fprintf(stdout, "|");
                    }
                    fprintf(stdout, "\n");
                    fprintf(stdout, "+..................................................................................................+\n");

                    // Restart the image transfer agent if it died.
                    if (ita != NULL &&  !ita->IsRunning()) {
                        delete ita;
                        ita = startImageTransferOnAThread(config);
                    }
                    // Collect our data.
                    try {
                        time_t start = time(NULL);
                        wm->GetLatestWeatherData();
                        gh->GetLatestSensorData();
                        time_t delta = time(NULL) - start;
                        if  (delta > 60)
                            sleepLen = 0;
                        else
                            sleepLen = 60 - delta;
                    } catch (std::exception& e) {
                        fprintf(stderr, "Exception Caught: %s", e.what());
                        sleepLen = 1;
                    }
                    fprintf(stdout, "+--------------------------------------------------------------------------------------------------+\n\n");
                    sleep(sleepLen);
                }
            }
        }
    }
    std::cout << "Usage " + std::string(argv[1]) + " <path to config.json>" << std::endl;
    return -1;
}

void *image_transfer_start_reoutine(void *x_void_ptr)
{
    APImageTransferAgent* ita = static_cast<APImageTransferAgent*>(x_void_ptr);
    try {
        ita->Run();
    } catch (std::exception& e) {
        fprintf(stderr, "Exception Caught in image transfer thread: %s", e.what());
    }

    return NULL;
}

APImageTransferAgent *startImageTransferOnAThread(Json::Value& config) {
    APImageTransferAgent* ita = new APImageTransferAgent(config);
    pthread_t image_transfer_thread;
    if(pthread_create(&image_transfer_thread, NULL, image_transfer_start_reoutine, ita)) {
        fprintf(stderr, "Error creating image transfer thread.\n");
        delete ita;
        return NULL;
    }
    return ita;
}