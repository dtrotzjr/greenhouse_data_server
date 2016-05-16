//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APSQLEXCEPTION_H
#define GREENHOUSE_DATA_SERVER_APSQLEXCEPTION_H
#include "APException.h"

class APSQLException : public APException {
public:
    APSQLException(std::string message);
};


#endif //GREENHOUSE_DATA_SERVER_APSQLEXCEPTION_H
