#ifndef _DBCONNECTION_HEADER_
#define _DBCONNECTION_HEADER_

#include <stdio.h>
#include <iostream>
#include <ctime>
#include <vector>
#include <set>
#include "DatabaseInterface.h"
#include "sqlite/sqlite3.h"
#include "Util/FileDatas.h"
#include <mutex>

class DatabaseConnection : public DatabaseInterface {
public:
    DatabaseConnection(const std::string& file);

    virtual ~DatabaseConnection() override;

    void initDb();

    bool createTables();
    bool truncateDatabase();

    bool addFile(const std::string& path);
    bool addCompilationUnit(const std::string& path, time_t d, const std::string& args);
    bool addRelation(const std::string& filePath, const std::string& depPath, time_t dependencyTimeStamp);

    std::vector<backport::FileDatas> getCompilationUnits();
    std::string getCmdLineForFile(const std::string& filePath);
    time_t getTimestampForFile(const std::string& filePath);
    std::vector<backport::FileDatas> getDependenciesList(const std::string& filePath);

    bool isFileInDb(const std::string& filePath);
    bool isCompilationUnitInDb(const std::string& filePath);

    bool removeAllDependencies(const std::string& filePath);

    bool updateRelation(const std::string& filePath, const std::string& depPath, time_t dependencyTimeStamp);
    bool updateTimestamp(const std::string& path, time_t date);
    bool updateCmdArguments(const std::string&, const std::string&);

    char* beginTransaction();
    char* endTransaction();


    bool updateLineOffsetMapping(std::string const &filepath, int actualLine, int originalLine);
    bool updateLineOffsetMapping(std::string const &filepath, long replacementId, int actualLine, int originalLine);
    long addReplacement(std::string const &filePath, short transformationId, int startLine, int numberOfLinesReplaced, int numberOfLinesInReplacementText, std::string const &fromPath, int from_start_line, int from_end_line, std::string const &replacement_text);
    std::vector<backport::ReplacementData> getReplacementsOfLine(std::string const& filePath, int line);
    std::set<short> getTransformationIdsOfLine(std::string const& filePath, int line);
    short getTransformationID(std::string const &transformationName);
    std::string getTransformationName(short transformationId);
    int getOriginalLineNumber(std::string const &filepath, int actualLine);
    backport::ReplacementData getReplacementById(long replacementid);
    int getLineId(std::string const& filePath, int line);
    bool updateLineNumberById(std::string const &filepath, int lineId, int newLineNumber, int originalLineNumber);
    int getActualLineNumberByLineId(int lineId);
    std::vector<backport::ReplacementData> getReplacementsWithStartLineLessThanOrEqual_raw(std::string const &filePath, int line);
    std::vector<backport::ReplacementData> getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw(std::string const &filePath, short transformationId, int line);
    std::vector<backport::ReplacementData> getUnhandledReplacements();
    bool setReplacementHandled(long replacementID);

    bool setTheNumberOfLinesForFile(std::string const &filePath, int lines);
    int getTheNumberOfLinesOfFile(std::string const &filePath);
    bool isActualLineInTheDb(std::string const &filePath, int line);
    bool resetLineForFile(std::string const &filepath);

private:
    std::recursive_mutex operationIsInProgressMutex;

    bool removeAllDependencies(int id);
    bool removeDependency(int id, int f_id);
    bool removeTables();

    bool runQuery(const char* q, const char* type);
    bool runQuery(sqlite3_stmt *, const char* type, std::set<int> const& ignoreError = {});
    bool runRawQuery(const std::string& query);

    bool isCompilationUnitInDb(int id);

    bool addCompilationUnit(int id, time_t d, const std::string& args);
    bool addRelation(int id_t1, int id_t2, time_t date);

    bool updateRelation(int id_t1, int id_t2, time_t date);
    bool updateTimestamp(int id, time_t d);
    bool updateCmdArguments(int id, const std::string& arg);

    int getIdForFile(const std::string& filepath);
    std::string getFilePath(int file_id);
    std::string getCmdLineForFile(int id);
    time_t getTimestampForFile(int id);
    std::vector<backport::FileDatas> getDependenciesList(int id);

    bool updateLineOffsetMapping(int file_id, int actualLine, int originalLine);
    bool updateLineOffsetMapping(int file_id, long replacementId, int actualLine, int originalLine);
    long addReplacement(int file_id, short transformationId, int startLine, int numberOfLinesReplaced, int numberOfLinesInReplacementText, int from_file_id, int from_start_line, int from_end_line, std::string const &replacement_text);
    std::vector<backport::ReplacementData> getReplacementsOfLine(int file_id, int line);
    std::set<short> getTransformationIdsOfLine(int file_id, int line);
    int getOriginalLineNumber(int file_id, int actualLine);
    int getLineId(int file_id, int line);
    bool updateLineNumberById(int file_id, int lineId, int newLineNumber, int originalLineNumber);
    std::vector<backport::ReplacementData> getReplacementsWithStartLineLessThanOrEqual_raw(int file_id, int line);
    std::vector<backport::ReplacementData> getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw(int file_id, short transformationId, int line);
    bool isActualLineInTheDb(int file_id, int line);

    bool setTheNumberOfLinesForFile(int file_id, int lines);
    int getTheNumberOfLinesOfFile(int file_id);
    bool resetLineForFile(int file_id);

    static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
        int i;
        for (i = 0; i < argc; i++){
            printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
        }
        printf("\n");
        return 0;
    }

    std::string dbPath;
    sqlite3 *database = 0;

    sqlite3_stmt *getDependencyList_query = 0;
    sqlite3_stmt *getCmdLineForFile_query = 0;
    sqlite3_stmt *getTimestampForFile_query = 0;
    sqlite3_stmt *getIdForFile_query = 0;
    sqlite3_stmt *getFilePath_query = 0;
    sqlite3_stmt *getCompilationUnits_query = 0;
    sqlite3_stmt *getLineId_query = 0;

    sqlite3_stmt *updateRelation_query = 0;
    sqlite3_stmt *updateCompUnitTimeStamp_query = 0;
    sqlite3_stmt *updateCmdArgs_query = 0;

    sqlite3_stmt *addCompUnit_query = 0;
    sqlite3_stmt *addRelation_query = 0;
    sqlite3_stmt *addFile_query = 0;

    sqlite3_stmt *existsCompilationUnit_query = 0;
    sqlite3_stmt *removeAllDependency_query = 0;

    /*=============================================*/
    sqlite3_stmt *updateLineOffsetMapping_query = 0;
    sqlite3_stmt *getReplacementById_query = 0;
    sqlite3_stmt *lines_relations_query = 0;
    sqlite3_stmt *addReplacement_insert_query = 0;
    sqlite3_stmt *addReplacement_select_query = 0;
    sqlite3_stmt *getReplacementsOfLine_query = 0;
    sqlite3_stmt *getTransformationID_insert_query = 0;
    sqlite3_stmt *getTransformationID_select_query = 0;
    sqlite3_stmt *getTransformationName_query = 0;
    sqlite3_stmt *getOriginalLineNumber_query = 0;
    sqlite3_stmt *updateLineById_query = 0;
    sqlite3_stmt *getActualLineNumberByLineId_query = 0;
    sqlite3_stmt *getReplacementsWithStartLineLessThanOrEqual_raw_query = 0;
    sqlite3_stmt *getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query = 0;
    sqlite3_stmt *getUnhandledReplacements_query = 0;
    sqlite3_stmt *setReplacementHandled_query = 0;
    sqlite3_stmt *setTheNumberOfLinesForFile_query = 0;
    sqlite3_stmt *getTheNumberOfLinesOfFile_query = 0;
    sqlite3_stmt *isActualLineInTheDb_query = 0;
    sqlite3_stmt *resetLineForFile_query = 0;

    DatabaseConnection const &initQueries();
    bool closeDatabase();
    DatabaseConnection const &cfgDB();

    std::vector<sqlite3_stmt *> prepStmts;
};

#endif
