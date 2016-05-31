//
// Created by David Trotz on 5/15/16.
//

#include "APSimpleSQL.h"
#include "APSQLException.h"
#include "APKeyValuePair.h"
#include <sqlite3.h>
#include <vector>
#include <string.h>
#include <string>

APSimpleSQL::APSimpleSQL(std::string databaseFile) {
    _transactionDepth = 0;
    if (databaseFile.length() > 0) {
        char* zErrMsg = NULL;
        int rc = sqlite3_open(databaseFile.c_str(), &_db);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
            sqlite3_close(_db);
        }
        // Set the database encoding to UTF-8 (we don't care if it fails as this will only succeed the very first time
        // the database is created.
        sqlite3_exec(_db, "PRAGMA encoding=\"UTF-8\";", NULL, 0, &zErrMsg);
        sqlite3_exec(_db, "PRAGMA synchronous=FULL;", NULL, 0, &zErrMsg);
        sqlite3_exec(_db, "PRAGMA journal_mode=WAL;", NULL, 0, &zErrMsg);
    } else {
        throw;
    }
}

APSimpleSQL::~APSimpleSQL() {
    sqlite3_close(_db);
    printf("Closed database");
}

void APSimpleSQL::BeginTransaction() {
    if (_transactionDepth == 0) {
//        printf("BEGIN\n");
        DoSQL("BEGIN TRANSACTION");
    }
    _transactionDepth++;
}

void APSimpleSQL::EndTransaction() {
    try {
        _transactionDepth--;
        if (_transactionDepth == 0) {
//            printf("COMMIT\n");
            DoSQL("COMMIT TRANSACTION");
        }
    } catch (APSQLException& e) {
        _transactionDepth++; // We didn't actually change the commit level so we revert.
        throw e;
    }
}

void APSimpleSQL::RollbackTransaction() {
    try {
        _transactionDepth--;
//        printf("ROLLBACK\n");
        DoSQL("ROLLBACK TRANSACTION");
    } catch (APSQLException& e) {
        _transactionDepth++; // We didn't actually change the commit level so we revert.
        throw e;
    }
}

void APSimpleSQL::DoSQL(const char* sql) {
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
    int rc = sqlite3_step(stmt);                                                                    /* 3 */
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        char buff[2048];
        sprintf(buff,"ERROR stepping statement (%s): %s\n", sql, sqlite3_errmsg(_db));
        throw APSQLException(std::string(buff));
    }
}

void APSimpleSQL::BeginSelect(const char* sql) {
    sqlite3_prepare_v2(_db, sql, -1, &_select_stmt, NULL);
}

void APSimpleSQL::EndSelect() {
    sqlite3_finalize(_select_stmt);
}

bool APSimpleSQL::StepSelect() {
    bool step_result;
    if (_select_stmt != NULL) {
        int rc = sqlite3_step(_select_stmt);                                                                    /* 3 */
        if (rc == SQLITE_ROW) {
            step_result = true;
        } else if (rc == SQLITE_DONE) {
            step_result = false;
        } else {
            char buff[2048];
            sprintf(buff,"ERROR stepping statement: %s\n", sqlite3_errmsg(_db));
            throw APSQLException(std::string(buff));
        }
    }
    return step_result;
}

int64_t  APSimpleSQL::GetColAsInt64(int colIndex) {
    int64_t retval;
    if (_select_stmt != NULL) {
        retval = sqlite3_column_int64(_select_stmt, colIndex);
    } else {
        char buff[2048];
        sprintf(buff,"ERROR getting column value as int64: %s\n", sqlite3_errmsg(_db));
        throw APSQLException(std::string(buff));
    }
    return retval;
}

const unsigned char* APSimpleSQL::GetColAsString(int colIndex) {
    const unsigned char* retval;
    if (_select_stmt != NULL) {
        retval = sqlite3_column_text(_select_stmt, colIndex);
    } else {
        char buff[2048];
        sprintf(buff,"ERROR getting column value as int64: %s\n", sqlite3_errmsg(_db));
        throw APSQLException(std::string(buff));
    }
    return retval;
}

double APSimpleSQL::GetColAsDouble(int colIndex) {
    double retval;
    if (_select_stmt != NULL) {
        retval = sqlite3_column_double(_select_stmt, colIndex);
    } else {
        char buff[2048];
        sprintf(buff,"ERROR getting column value as int64: %s\n", sqlite3_errmsg(_db));
        throw APSQLException(std::string(buff));
    }
    return retval;
}

int64_t APSimpleSQL::DoInsert(const char* sql) {
    int64_t lastId = -1;
    DoSQL(sql);
    lastId = sqlite3_last_insert_rowid(_db);
    return lastId;
}

int64_t APSimpleSQL::DoInsert(const char* tableName, std::vector<APKeyValuePair*>* pairs) {
    int buffLen = 32768;
    char buff[buffLen];
    snprintf(buff, buffLen, "INSERT INTO %s (", tableName);
    for (std::vector<APKeyValuePair*>::iterator it = pairs->begin() ; it != pairs->end(); ++it) {
        if (it != pairs->begin()) {
            strncat(buff, ",", buffLen);
        }
        strncat(buff, (*it)->GetKey().c_str(), buffLen);
    }
    strncat(buff, ") VALUES (", buffLen);
    for (std::vector<APKeyValuePair*>::iterator it = pairs->begin() ; it != pairs->end(); ++it) {
        if (it != pairs->begin()) {
            strncat(buff, ",", buffLen);
        }
        strncat(buff, "'", buffLen);
        strncat(buff, (*it)->GetValue().c_str(), buffLen);
        strncat(buff, "'", buffLen);
    }
    strncat(buff, ");", buffLen);
//    printf("\n%s\n", buff);
    return DoInsert(buff);
}

bool APSimpleSQL::RowExists(const char* table_name, int64_t rowid) {
    bool exists = false;
    sqlite3_stmt* stmt;
    char buff[4096];
    snprintf(buff, 4096, "select exists (select 1 from %s where id = %lld)", table_name, rowid);
    sqlite3_prepare_v2(_db, buff, -1, &stmt, NULL);
    int rc = sqlite3_step(stmt);                                                                    /* 3 */

    if (rc == SQLITE_ROW) {
        exists = sqlite3_column_int(stmt,0) == 1;
    } else {
        char buff[2048];
        sprintf(buff,"ERROR with row exists statement: %s\n", sqlite3_errmsg(_db));
        sqlite3_finalize(stmt);
        throw APSQLException(std::string(buff));
    }
    sqlite3_finalize(stmt);
    return exists;
}

int64_t APSimpleSQL::DoUpdate(const char* tableName, std::vector<APKeyValuePair*>* pairs, char* where) {
    int buffLen = 32768;
    char buff[buffLen];
    snprintf(buff, buffLen, "UPDATE %s SET ", tableName);
    for (std::vector<APKeyValuePair*>::iterator it = pairs->begin() ; it != pairs->end(); ++it) {
        if (it != pairs->begin()) {
            strncat(buff, ",", buffLen);
        }
        strncat(buff, (*it)->GetKey().c_str(), buffLen);
        strncat(buff, "=", buffLen);
        strncat(buff, (*it)->GetValue().c_str(), buffLen);
    }
    strncat(buff, " WHERE ", buffLen);
    strncat(buff, where, buffLen);
    strncat(buff, ";", buffLen);
    printf("\n%s\n", buff);
    int total_before = sqlite3_total_changes(_db);
    DoSQL(buff);
    int total_after = sqlite3_total_changes(_db);
    return (int64_t)(total_after - total_before);
}