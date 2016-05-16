//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APEXCEPTION_H
#define GREENHOUSE_DATA_SERVER_APEXCEPTION_H

#include <exception>
#include <string>

class APException : public std::exception {
public:
    APException(std::string message);
    virtual const char* what() const throw();
private:
    std::string _message;
};


#endif //GREENHOUSE_DATA_SERVER_APEXCEPTION_H
