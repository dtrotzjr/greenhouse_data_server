//
// Created by David Trotz on 5/15/16.
//

#include "APSimpleJSONQuery.h"
#include <stdlib.h>
#include <curl/curl.h>
#include <string>
#include <memory.h>

static size_t _writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp);

APSimpleJSONQuery::APSimpleJSONQuery()  {
    _curlResponseChunk.buffer = NULL;
    _curlResponseChunk.size = 0;
}

APSimpleJSONQuery::~APSimpleJSONQuery()  {
    free(_curlResponseChunk.buffer);
}

char* APSimpleJSONQuery::GetJSONResponseFromURL(const char* url) {
    CURL *curl;
    CURLcode res;
    if(_curlResponseChunk.buffer != NULL) {
        free(_curlResponseChunk.buffer);
    }
    _curlResponseChunk.buffer = (char*)malloc(1);  /* will be grown as needed by the realloc above */
    _curlResponseChunk.size = 0;    /* no data at this point */

    curl = curl_easy_init();

    if (curl != NULL) {
        fprintf(stdout, "Trying: %s\n", url);
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, _writeMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&_curlResponseChunk);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        }
        curl_easy_cleanup(curl);
    }
    return _curlResponseChunk.buffer;
}

static size_t
_writeMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    JSONResponseStructType *mem = (JSONResponseStructType *)userp;

    mem->buffer = (char*)realloc(mem->buffer, mem->size + realsize + 1);
    if(mem->buffer == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->buffer[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->buffer[mem->size] = 0;

    return realsize;
}