//
// Created by David Trotz on 5/15/16.
//

#include "APSimpleSQL.h"
#include "APSQLException.h"
#include <sqlite3.h>

APSimpleSQL::APSimpleSQL(std::string databaseFile) {
    if (databaseFile.length() > 0) {
        char* zErrMsg = NULL;
        int rc = sqlite3_open(databaseFile.c_str(), &_db);
        if (rc != SQLITE_OK) {
            fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(_db));
            sqlite3_close(_db);
        }
        // Set the database encoding to UTF-8 (we don't care if it fails as this will only succeed the very first time
        // the database is created.
        rc = sqlite3_exec(_db, "PRAGMA encoding = \"UTF-8\";", NULL, 0, &zErrMsg);
    } else {
        throw;
    }
}

APSimpleSQL::~APSimpleSQL() {
    sqlite3_close(_db);
}

void APSimpleSQL::BeginTransaction() {
    if (_transactionDepth == 0) {
        DoSQL("BEGIN TRANSACTION");
    }
    _transactionDepth--;
}

void APSimpleSQL::EndTransaction() {
    try {
        _transactionDepth--;
        if (_transactionDepth == 0) {
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
        if (_transactionDepth == 0) {
            DoSQL("ROLLBACK TRANSACTION");
        }
    } catch (APSQLException& e) {
        _transactionDepth++; // We didn't actually change the commit level so we revert.
        throw e;
    }
}

void APSimpleSQL::DoSQL(const char* sql) {
    bool failed = false;
    sqlite3_stmt* stmt;
    sqlite3_prepare_v2(_db, sql, -1, &stmt, NULL);
    int rc = sqlite3_step(stmt);                                                                    /* 3 */
    sqlite3_finalize(stmt);
    if (rc != SQLITE_DONE) {
        char buff[2048];
        sprintf(buff,"ERROR stepping statement: %s\n", sqlite3_errmsg(_db));
        throw APSQLException(std::string(buff));
    }
}