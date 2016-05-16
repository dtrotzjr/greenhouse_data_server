//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APSIMPLESQL_H
#define GREENHOUSE_DATA_SERVER_APSIMPLESQL_H
#include <string>
struct sqlite3;

class APSimpleSQL {
public:
    APSimpleSQL(std::string databaseFile);
    virtual ~APSimpleSQL();

    void BeginTransaction();
    void EndTransaction();
    void RollbackTransaction();

    void DoSQL(const char* sql);

private:
    sqlite3 *_db;

    int _transactionDepth;
};


#endif //GREENHOUSE_DATA_SERVER_APSIMPLESQL_H
