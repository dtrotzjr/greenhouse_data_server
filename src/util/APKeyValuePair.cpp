//
// Created by David Trotz on 5/17/16.
//

#include "APKeyValuePair.h"
#include <stdio.h>

APKeyValuePair::APKeyValuePair(std::string key, std::string value) {
    _key = key;
    _value = value;
}

APKeyValuePair::APKeyValuePair(std::string key, int64_t value) {
    _key = key;
    char buff[256];
    snprintf(buff, 256, "%lld", value);
    _value =  buff;
}

APKeyValuePair::APKeyValuePair(std::string key, double value) {
    _key = key;
    char buff[256];
    snprintf(buff, 256, "%lf", value);
    _value =  buff;
}

APKeyValuePair::~APKeyValuePair() {

}

std::string APKeyValuePair::GetKey() {
    return _key;
}

std::string APKeyValuePair::GetValue() {
    return _value;
}