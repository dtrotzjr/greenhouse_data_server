//
// Created by David Trotz on 5/15/16.
//

#include "APException.h"

APException::APException(std::string message) {
    _message = message;
}

const char* APException::what() const throw() {
    return _message.c_str();
}