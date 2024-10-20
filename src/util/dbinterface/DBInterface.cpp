/**
Copyright 2024 JasmineGraph Team
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
 */

#include "DBInterface.h"
#include "../logger/Logger.h"

Logger interface_logger;

int DBInterface::finalize() {
    return sqlite3_close(database);
}

typedef vector<vector<pair<string, string>>> table_type;

static int callback(void *ptr, int argc, char **argv, char **columnName) {
    table_type *dbResults = static_cast<table_type *>(ptr);
    vector<pair<string, string>> results;

    for (int i = 0; i < argc; i++) {
        results.push_back(make_pair(columnName[i], argv[i] ? argv[i] : "NULL"));
    }
    dbResults->push_back(results);
    return 0;
}

vector<vector<pair<string, string>>> DBInterface::runSelect(string query) {

//    char *errorMessage = 0;
//    vector<vector<pair<string, string>>> dbResults;
//
//    if (sqlite3_exec(database, query.c_str(), callback, &dbResults, &errorMessage) != SQLITE_OK) {
//        interface_logger.info(query);
//        interface_logger.error("SQL Error: " + string(errorMessage) + " " + query);
//        sqlite3_free(errorMessage);
//    }

    vector<vector<pair<string, string>>> dbResults;

    char* errorMessage = nullptr;

    int rc = sqlite3_exec(database, query.c_str(), callback, &dbResults, &errorMessage);

    if (rc != SQLITE_OK) {
        if (errorMessage != nullptr) {
            // Log the error message from sqlite3_exec
            interface_logger.error("SQL Error: " + string(errorMessage) + " " + query);
            sqlite3_free(errorMessage);
            errorMessage = nullptr;
        } else {

            interface_logger.error("SQL Error: " + string(sqlite3_errmsg(database)) + " " + query);
        }
    }
    return dbResults;
}

// This function inserts a new row to the DB and returns the last inserted row id
// returns -1 on error
int DBInterface::runInsert(std::string query) {
    char *errorMessage = 0;
    int rc = sqlite3_exec(database, query.c_str(), NULL, NULL, &errorMessage);
    if (rc != SQLITE_OK) {
        interface_logger.error("SQL Error: " + string(errorMessage) + " " + query);
        sqlite3_free(errorMessage);
        return -1;
    }
    vector<vector<pair<string, string>>> dbResults;
    string q2 = "SELECT last_insert_rowid();";

    int rc2 = sqlite3_exec(database, q2.c_str(), callback, &dbResults, &errorMessage);

    if (rc2 != SQLITE_OK) {
        interface_logger.error("SQL Error: " + string(errorMessage) + " " + query);
        sqlite3_free(errorMessage);
        return -1;
    }

    if (dbResults.empty() || dbResults[0].empty()) {
        return -1;
    }
    return std::stoi(dbResults[0][0].second);
}

// This function inserts one or more rows of the DB and nothing is returned
// This is used for inserting tables which do not have primary IDs
void DBInterface::runInsertNoIDReturn(std::string query) {
    char *errorMessage = 0;
    int rc = sqlite3_exec(database, query.c_str(), NULL, NULL, &errorMessage);
    if (rc != SQLITE_OK) {
        interface_logger.error("SQL Error: " + string(errorMessage) + " " + query);
        sqlite3_free(errorMessage);
    }
}

// This function updates one or more rows of the DB
void DBInterface::runUpdate(std::string query) {
    char *errorMessage = 0;

    int rc = sqlite3_exec(database, query.c_str(), NULL, NULL, &errorMessage);

    if (rc != SQLITE_OK) {
        interface_logger.error("SQL Error: " + string(errorMessage) + " " + query);
        sqlite3_free(errorMessage);
    }
}

int DBInterface::runSqlNoCallback(const char *zSql) {
    sqlite3_stmt *stmt = NULL;
    int rc = sqlite3_prepare_v2(database, zSql, -1, &stmt, NULL);
    if (rc != SQLITE_OK) return rc;

    int rowCount = 0;
    rc = sqlite3_step(stmt);
    while (rc != SQLITE_DONE && rc != SQLITE_OK) {
        rowCount++;
        int colCount = sqlite3_column_count(stmt);
        for (int colIndex = 0; colIndex < colCount; colIndex++) {
            int type = sqlite3_column_type(stmt, colIndex);
            const char *columnName = sqlite3_column_name(stmt, colIndex);
            if (type == SQLITE_INTEGER) {
                int valInt = sqlite3_column_int(stmt, colIndex);
            } else if (type == SQLITE_FLOAT) {
                double valDouble = sqlite3_column_double(stmt, colIndex);
            } else if (type == SQLITE_TEXT) {
                const unsigned char *valChar = sqlite3_column_text(stmt, colIndex);
            }
        }
        interface_logger.info("Line " + std::to_string(rowCount) + ", rowCount " + std::to_string(colCount));

        rc = sqlite3_step(stmt);
    }

    rc = sqlite3_finalize(stmt);

    return rc;
}
