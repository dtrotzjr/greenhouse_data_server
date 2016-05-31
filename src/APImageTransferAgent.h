//
// Created by David Trotz on 5/30/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APIMAGETRANSFERAGENT_H
#define GREENHOUSE_DATA_SERVER_APIMAGETRANSFERAGENT_H
#include <libssh/sftp.h>
#include <json/json.h>
#include <string>
#include <vector>
#include <set>
class APSimpleSQL;

class APImageTransferAgent {
public:
    APImageTransferAgent(Json::Value config);
    ~APImageTransferAgent();
    void Run();
    bool IsRunning();
private:
    sftp_session _sftp;
    APSimpleSQL *_sqlDb;
    Json::Value _config;
    bool _running;

    bool _step();
    std::vector<std::string> _splitpath(const std::string& str, const std::set<char> delimiters);
    bool _directoryExists(std::string path);
    ssh_session _ssh_connect(const char *host, const char *port, const char *user,int verbosity);
    int _sftp_image(std::string remote_filename, std::string local_filename);
    int _ssh_verify_knownhost(ssh_session session);
    int _ssh_authenticate(ssh_session session);
};


#endif //GREENHOUSE_DATA_SERVER_APIMAGETRANSFERAGENT_H
