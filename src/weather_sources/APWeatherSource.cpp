//
// Created by David Trotz on 5/15/16.
//

#include "APWeatherSource.h"
#include "APSimpleJSONQuery.h"


APWeatherSource::APWeatherSource() {
    _jsonQueryObj = new APSimpleJSONQuery();
};

APWeatherSource::~APWeatherSource() {
    delete _jsonQueryObj;
};