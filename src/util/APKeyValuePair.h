//
// Created by David Trotz on 5/17/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APKEYVALUEPAIR_H
#define GREENHOUSE_DATA_SERVER_APKEYVALUEPAIR_H


class APKeyValuePair {
    APKeyValuePair(std::string key, std::string value);
    APKeyValuePair(std::string key, int64_t value);
    APKeyValuePair(std::string key, double value);
    ~APKeyValuePair();

    std::string GetKey();
    std::string GetValue();
protected:
    std::string _key;
    std::string _value;
};


#endif //GREENHOUSE_DATA_SERVER_APKEYVALUEPAIR_H
