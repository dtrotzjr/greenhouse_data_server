//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APSIMPLEJSONQUERY_H
#define GREENHOUSE_DATA_SERVER_APSIMPLEJSONQUERY_H
#include <stdlib.h>

typedef struct JSONResponseStruct {
    char *buffer;
    size_t size;
    bool success;
} JSONResponseStructType;

class APSimpleJSONQuery {
public:
    APSimpleJSONQuery();
    virtual ~APSimpleJSONQuery();
    JSONResponseStructType GetJSONResponseFromURL(const char* url);

private:
    JSONResponseStructType _curlResponseChunk;

};


#endif //GREENHOUSE_DATA_SERVER_APSIMPLEJSONQUERY_H
