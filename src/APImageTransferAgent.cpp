//
// Created by David Trotz on 5/30/16.
//

#include "APImageTransferAgent.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <errno.h>
#include <error.h>
#include <string>
#include <vector>
#include <ostream>
#include <set>
#include <libssh/libssh.h>
#include <libssh/sftp.h>
#include <fcntl.h>
#include <json/json.h>
#include <unistd.h>
#include "APSimpleSQL.h"
#define DATALEN 65536


// Note: Much of the ssh code here is adapted from libssh's examples

APImageTransferAgent::APImageTransferAgent(Json::Value config) {
    _config = config;
    std::string databaseFile = config["sqlite3_file"].asString();
    _sqlDb = new APSimpleSQL(databaseFile);
    _running = false;
}

APImageTransferAgent::~APImageTransferAgent() {
    delete _sqlDb;
}

void APImageTransferAgent::Run() {
    _running = true;
    bool done = false;
    while(!done) {
        done = _step();
    }
    _running = false;
}

bool APImageTransferAgent::_step() {
    bool done = false;
    if (_sqlDb != NULL) {
        // Get the next oldest image that has not been synchronized
        int64_t rowid = -1;
        std::string fullPathname = "";
        int64_t data_points_id = -1;
        _sqlDb->BeginSelect("SELECT gh_image_data.id,gh_image_data.filename,MIN(gh_data_points.timestamp) as timestamp,gh_data_points.id as gh_data_points_id FROM gh_image_data JOIN gh_data_points ON gh_image_data.gh_data_point_id = gh_data_points.id WHERE gh_data_points.synchronized == 0");
        if (_sqlDb->StepSelect()) {
            rowid = _sqlDb->GetColAsInt64(0);
            fullPathname = std::string((const char *) _sqlDb->GetColAsString(1));
            data_points_id = _sqlDb->GetColAsInt64(3);
        }
        _sqlDb->EndSelect();
        if (rowid != -1) {
            std::set<char> delims{'/'};
            std::vector<std::string> pathParts = _splitpath(fullPathname, delims);
            std::string destinationPathname;
            if (_config["images_destination"] != Json::Value::null) {
                destinationPathname = _config["images_destination"].asString();
            }
            std::string destinationPathnameFallback;
            if (_config["images_destination_fallback"] != Json::Value::null) {
                destinationPathnameFallback = _config["images_destination_fallback"].asString();
            }

            std::string filename = pathParts.back();

            char* fullDestFilename = NULL;

            if (destinationPathname.length() > 0 && _directoryExists(destinationPathname)) {
                int fullDestFilenameLen = filename.length() + destinationPathname.length() + 2;
                fullDestFilename = new char[fullDestFilenameLen];
                snprintf(fullDestFilename, fullDestFilenameLen, "%s/%s", destinationPathname.c_str(), filename.c_str());
            } else if (destinationPathnameFallback.length() > 0 && _directoryExists(destinationPathnameFallback)) {
                int fullDestFilenameLen = filename.length() + destinationPathnameFallback.length() + 2;
                fullDestFilename = new char[fullDestFilenameLen];
                snprintf(fullDestFilename, fullDestFilenameLen, "%s/%s", destinationPathnameFallback.c_str(), filename.c_str());
            }

            if (fullDestFilename != NULL) {
                if(_sftp_image(fullPathname.c_str(), std::string(fullDestFilename)) == 0) {
                    std::vector<APKeyValuePair*> *pairs = new std::vector<APKeyValuePair*>();
                    APKeyValuePair* pair = new APKeyValuePair("synchronized", (int64_t)1);
                    pairs->push_back(pair);
                    int whereLen = 256;
                    char where[whereLen];
                    snprintf(where, whereLen, "id = %lld", data_points_id);
                    _sqlDb->BeginTransaction();
                    int updatedRows = _sqlDb->DoUpdate("gh_data_points", pairs, where);
                    _sqlDb->EndTransaction();
                    if(updatedRows == 1) {
                        fprintf(stdout, "Copied file and updated database successfully.");
                    } else {
                        fprintf(stderr, "Updated gh_data_points row count mismatch. Wanted 1 got %d", updatedRows);
                        done = true;
                    }
                }
            } else {
                fprintf(stderr, "ERROR: Neither 'images_destination' nor 'images_destination_fallback' appear to be accessible.\n");
                done = true;
            }
        }
    }
    return done;
}

bool APImageTransferAgent::_directoryExists(std::string path) {

    bool exists = false;
    if (path[path.length() - 1] != '/') {
        path += "/";
    }
    if(0 == access(path.c_str(), F_OK)) {
        exists = true;
    }
    return exists;
}

// Taken from http://stackoverflow.com/questions/8520560/get-a-file-name-from-a-path
std::vector<std::string> APImageTransferAgent::_splitpath(const std::string& str, const std::set<char> delimiters) {
    std::vector<std::string> result;

    char const* pch = str.c_str();
    char const* start = pch;
    for(; *pch; ++pch)
    {
        if (delimiters.find(*pch) != delimiters.end())
        {
            if (start != pch)
            {
                std::string str(start, pch);
                result.push_back(str);
            }
            else
            {
                result.push_back("");
            }
            start = pch + 1;
        }
    }
    result.push_back(start);

    return result;
}

bool APImageTransferAgent::IsRunning() {
    return _running;
}

int APImageTransferAgent::_sftp_image(std::string remote_filename, std::string local_filename) {
    ssh_session session;
    char data[DATALEN]={0};
    sftp_file remote_file_in;
    FILE* local_file_out;
    int len=1;
    unsigned int i;


    if(_config["greenhouse_server"] != Json::Value::null) {
        std::string server_ip = _config["greenhouse_server"].asString();
        std::string server_port = "22";
        std::string server_username = "pi";
        if(_config["greenhouse_server_ssh_port"] != Json::Value::null) {
            server_port = _config["greenhouse_server_ssh_port"].asString();
        }
        if(_config["greenhouse_server_username"] != Json::Value::null) {
            server_username = _config["greenhouse_server_username"].asString();
        }

        session = _ssh_connect(server_ip.c_str(),server_port.c_str(),server_username.c_str(),0);

        if(session == NULL)
            return EXIT_FAILURE;

        sftp_session sftp=sftp_new(session);

        if (!sftp) {
            fprintf(stderr, "sftp error initialising channel: %s\n", ssh_get_error(session));
            return SSH_ERROR;
        }
        if (sftp_init(sftp) != 0) {
            fprintf(stderr, "error initialising sftp: %s\n",
                    ssh_get_error(session));
            return SSH_ERROR;
        }


        remote_file_in = sftp_open(sftp,remote_filename.c_str(),O_RDONLY, 0);
        if (remote_file_in == NULL) {
            fprintf(stderr, "Error opening %s: %s\n", remote_filename.c_str(), ssh_get_error(session));
            return SSH_ERROR;
        }
        /* open a file for writing... */

        local_file_out=fopen(local_filename.c_str(),"w");
        if (local_file_out == NULL) {
            fprintf(stderr, "Error opening destination file for writing\n");
            return SSH_ERROR;
        }
        while ((len=sftp_read(remote_file_in,data,4096)) > 0) {
            if (fwrite(data,1,len,local_file_out)!=len) {
                fprintf(stderr, "Error writing %d bytes\n",
                        len);
                return SSH_ERROR;
            }
        }
        printf("finished\n");
        if (len < 0)
            fprintf(stderr, "Error reading file: %s\n", ssh_get_error(session));
        sftp_close(remote_file_in);
        fclose(local_file_out);

        //*********************************************************************************************************************


        int rc;
        _sftp = sftp_new(session);
        if (_sftp == NULL) {
            fprintf(stderr, "Error allocating SFTP session: %s\n", ssh_get_error(session));
            return SSH_ERROR;
        }
        rc = sftp_init(_sftp);
        if (rc != SSH_OK) {
            fprintf(stderr, "Error initializing SFTP session: %s.\n", sftp_get_error(_sftp));
            sftp_free(_sftp);
            return rc;
        }

        // Cleanup
        sftp_free(_sftp);
        ssh_disconnect(session);
        ssh_free(session);

        return SSH_OK;
    }
}

ssh_session APImageTransferAgent::_ssh_connect(const char *host, const char *port, const char *user,int verbosity) {
    ssh_session session;
    int auth = 0;

    session = ssh_new();
    if (session == NULL) {
        return NULL;
    }

    if(user != NULL){
        if (ssh_options_set(session, SSH_OPTIONS_USER, user) < 0) {
            ssh_disconnect(session);
            return NULL;
        }
    }

    if (ssh_options_set(session, SSH_OPTIONS_HOST, host) < 0) {
        return NULL;
    }

    if (ssh_options_set(session, SSH_OPTIONS_PORT_STR, port) < 0) {
        return NULL;
    }

    ssh_options_set(session, SSH_OPTIONS_LOG_VERBOSITY, &verbosity);
    if (ssh_connect(session)) {
        fprintf(stderr,"Connection failed : %s\n",ssh_get_error(session));
        ssh_disconnect(session);
        return NULL;
    }
    if (_ssh_verify_knownhost(session) < 0) {
        ssh_disconnect(session);
        return NULL;
    }
    auth = _ssh_authenticate(session);
    if (auth == SSH_AUTH_SUCCESS) {
        return session;
    } else if (auth == SSH_AUTH_DENIED) {
        fprintf(stderr,"Authentication failed\n");
    } else {
        fprintf(stderr,"Error while authenticating : %s\n",ssh_get_error(session));
    }
    ssh_disconnect(session);
    return NULL;
}

int APImageTransferAgent::_ssh_verify_knownhost(ssh_session session) {
    char *hexa;
    int state;
    char buf[10];
    unsigned char *hash = NULL;
    size_t hlen;
    int rc;
    ssh_key srv_pubkey;

    state=ssh_is_server_known(session);

    rc = ssh_get_publickey(session, &srv_pubkey);
    if (rc < 0) {
        return -1;
    }

    rc = ssh_get_publickey_hash(srv_pubkey,
                                SSH_PUBLICKEY_HASH_SHA1,
                                &hash,
                                &hlen);

    if (hlen < 0) {
        return -1;
    }
    switch(state){
        case SSH_SERVER_KNOWN_OK:
            break; /* ok */
        case SSH_SERVER_KNOWN_CHANGED:
            fprintf(stderr,"Host key for server changed : server's one is now :\n");
            ssh_print_hexa("Public key hash",hash, hlen);
            free(hash);
            fprintf(stderr,"For security reason, connection will be stopped\n");
            return -1;
        case SSH_SERVER_FOUND_OTHER:
            fprintf(stderr,"The host key for this server was not found but an other type of key exists.\n");
            fprintf(stderr,"An attacker might change the default server key to confuse your client"
                    "into thinking the key does not exist\n"
                    "We advise you to rerun the client with -d or -r for more safety.\n");
            return -1;
        case SSH_SERVER_FILE_NOT_FOUND:
            fprintf(stderr,"Could not find known host file. If you accept the host key here,\n");
            fprintf(stderr,"the file will be automatically created.\n");
            /* fallback to SSH_SERVER_NOT_KNOWN behavior */
        case SSH_SERVER_NOT_KNOWN:
            hexa = ssh_get_hexa(hash, hlen);
            fprintf(stderr,"The server is unknown. Do you trust the host key ?\n");
            fprintf(stderr, "Public key hash: %s\n", hexa);
            free(hexa);
            if (fgets(buf, sizeof(buf), stdin) == NULL) {
                return -1;
            }
            if(strncasecmp(buf,"yes",3)!=0){
                return -1;
            }
            fprintf(stderr,"This new key will be written on disk for further usage. do you agree ?\n");
            if (fgets(buf, sizeof(buf), stdin) == NULL) {
                return -1;
            }
            if(strncasecmp(buf,"yes",3)==0){
                if (ssh_write_knownhost(session) < 0) {
                    free(hash);
                    fprintf(stderr, "error %s\n", strerror(errno));
                    return -1;
                }
            }

            break;
        case SSH_SERVER_ERROR:
            free(hash);
            fprintf(stderr,"%s",ssh_get_error(session));
            return -1;
    }
    free(hash);
    return 0;
}

int APImageTransferAgent::_ssh_authenticate(ssh_session session) {
    int rc;
    int method;
    char password[128] = {0};
    char *banner;

    // Try to authenticate
    rc = ssh_userauth_none(session, NULL);
    if (rc == SSH_AUTH_ERROR) {
        return rc;
    }

    method = ssh_auth_list(session);
    while (rc != SSH_AUTH_SUCCESS) {
        // Try to authenticate with public key first
        if (method & SSH_AUTH_METHOD_PUBLICKEY) {
            rc = ssh_userauth_autopubkey(session, NULL);
            if (rc == SSH_AUTH_ERROR) {
                return rc;
            } else if (rc == SSH_AUTH_SUCCESS) {
                break;
            }
        }
    }

    banner = ssh_get_issue_banner(session);
    if (banner) {
        printf("%s\n",banner);
        free(banner);
    }

    return rc;
}