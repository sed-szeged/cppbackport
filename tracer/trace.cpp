#define _CRT_SECURE_NO_WARNINGS
#include "DatabaseConnection.h"
#include <iostream>
#include <cstdlib>
#include <string>
#include <set>
#include <sstream>
#include <cstring>

extern "C" {
    int traceBack(char const *dbFilePath, char const *filePath, int lineNum, int pathIsCorrect) {

        DatabaseConnection db(dbFilePath);
        db.initDb();

        if (db.isFileInDb(filePath) == false) {
            if (pathIsCorrect) {
                return lineNum;
            }

            return -1;
        }

        return db.getOriginalLineNumber(filePath, lineNum);
    }
}
