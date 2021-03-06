//
// Created by David Trotz on 5/15/16.
//

#ifndef GREENHOUSE_DATA_SERVER_APSIMPLESQL_H
#define GREENHOUSE_DATA_SERVER_APSIMPLESQL_H
#include <string>
#include <sqlite3.h>
#include <vector>
#include "APKeyValuePair.h"

struct sqlite3;
class APKeyValuePair;

class APSimpleSQL {
public:
    APSimpleSQL(std::string databaseFile);
    virtual ~APSimpleSQL();

    void BeginTransaction();
    void EndTransaction();
    void RollbackTransaction();

    void DoSQL(const char* sql);
    int64_t DoInsert(const char* sql);
    int64_t DoInsert(const char* tableName, std::vector<APKeyValuePair*>* pairs);
    int64_t DoUpdate(const char* tableName, std::vector<APKeyValuePair*>* pairs, char* where);

    bool RowExists(const char* table_name, int64_t rowid);

    void BeginSelect(const char* sql);
    bool StepSelect();
    int64_t  GetColAsInt64(int colIndex);
    double GetColAsDouble(int colIndex);
    const unsigned char* GetColAsString(int colIndex);
    void EndSelect();
private:
    sqlite3 *_db;
    sqlite3_stmt *_select_stmt;
    int _transactionDepth;
    std::string _selectStmtString;
};


#endif //GREENHOUSE_DATA_SERVER_APSIMPLESQL_H
