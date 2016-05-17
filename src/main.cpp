#include <iostream>
#include "APWeatherDataManager.h"
#include <unistd.h>
#include "APException.h"

int main(int argc, char* argv[]) 
{
    if (argc == 2) {
        APWeatherDataManager* wm = new APWeatherDataManager(argv[1]);
        while(true) {
            try {
                wm->GetLatestWeatherData();
                sleep(60);
            } catch (APException& e) {
                break;
            }
        }
        printf("Abored!");
        delete wm;
        return 0;
    }
    std::cout << "Usage " + std::string(argv[1]) + " <path to config.json>" << std::endl;
    return -1;
}
