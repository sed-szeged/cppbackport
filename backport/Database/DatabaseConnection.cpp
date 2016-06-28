#include <string>
#include <ctime>
#include "DatabaseConnection.h"
#include "Util/FileDatas.h"
#include "Util/Log.h"
#include "sqlite/sqlite3.h"

DatabaseConnection::DatabaseConnection(const std::string& file) : dbPath(file) {
    prepStmts =
    {
        getDependencyList_query, getCmdLineForFile_query, getTimestampForFile_query,
        getIdForFile_query, getCompilationUnits_query, updateRelation_query, updateCompUnitTimeStamp_query, updateCmdArgs_query,
        addCompUnit_query, addRelation_query, addFile_query, existsCompilationUnit_query, removeAllDependency_query,

        updateLineOffsetMapping_query, getReplacementById_query, lines_relations_query, addReplacement_insert_query, addReplacement_select_query,
        getReplacementsOfLine_query, getTransformationID_insert_query, getTransformationID_select_query, getTransformationName_query,
        getOriginalLineNumber_query, updateLineById_query, getActualLineNumberByLineId_query, getReplacementsWithStartLineLessThanOrEqual_raw_query,
        getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query, getUnhandledReplacements_query, setReplacementHandled_query,
        setTheNumberOfLinesForFile_query, getTheNumberOfLinesOfFile_query, isActualLineInTheDb_query, resetLineForFile_query
    };
}

DatabaseConnection::~DatabaseConnection() {
    for (auto stmt : prepStmts) {
        sqlite3_finalize(stmt);
    }

    if (this->database) {
        closeDatabase();
    }
}

void DatabaseConnection::initDb() {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    if (database) {
        return;
    }

    const int rc = sqlite3_open(dbPath.c_str(), &database);

    if (rc) {
        LOG(logERROR) << "Can't open database: " << sqlite3_errmsg(database);
    }
    else {
        // NOTE: Call order is really important!
        cfgDB();
        createTables();
        initQueries();
        LOG(logDEBUG) << "Database opened successfully!";
    }
}

bool DatabaseConnection::closeDatabase() {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    bool res = sqlite3_close(database) == SQLITE_OK;;
    database = 0;
    return res;
}

bool DatabaseConnection::truncateDatabase() {
    removeTables();
    return createTables();
}

DatabaseConnection const &DatabaseConnection::cfgDB() {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);
    sqlite3_soft_heap_limit64(0);
    sqlite3_soft_heap_limit(0);

    char* sErrMsg = nullptr;
    sqlite3_exec(database, "PRAGMA synchronous = OFF", NULL, NULL, &sErrMsg);
    sqlite3_exec(database, "PRAGMA journal_mode = MEMORY", NULL, NULL, &sErrMsg);
    sqlite3_exec(database, "PRAGMA busy_timeout = 100000;", NULL, NULL, &sErrMsg);

    return *this;
}

DatabaseConnection const &DatabaseConnection::initQueries() {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);
    std::string sql;
    sql = "SELECT FILES.path, RELATIONS.dependency_timestamp FROM FILES INNER JOIN RELATIONS on FILES.id = RELATIONS.dep_id WHERE RELATIONS.file_id = ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getDependencyList_query, NULL);

    sql = "UPDATE `RELATIONS` SET `dependency_timestamp` = ?1 WHERE `file_id` = ?2 AND `dep_id`= ?3;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &updateRelation_query, NULL);

    sql = "UPDATE `COMPILATION_UNIT` SET `timestamp` = ?1 WHERE `id` = ?2;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &updateCompUnitTimeStamp_query, NULL);

    sql = "UPDATE `COMPILATION_UNIT` SET `cmd_args` = ?1 WHERE `id` = ?2;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &updateCmdArgs_query, NULL);

    sql = "INSERT INTO `COMPILATION_UNIT`(`id`,`timestamp`,`cmd_args`) VALUES(?1, ?2, ?3);";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &addCompUnit_query, NULL);

    sql = "INSERT OR IGNORE INTO `RELATIONS`(`file_id`,`dep_id`,`dependency_timestamp`) VALUES(?1,  ?2, ?3);";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &addRelation_query, NULL);

    sql = "DELETE FROM `RELATIONS` WHERE `file_id` = ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &removeAllDependency_query, NULL);

    sql = "SELECT cmd_args FROM COMPILATION_UNIT WHERE id = ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getCmdLineForFile_query, NULL);

    sql = "SELECT id FROM FILES WHERE path = ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getIdForFile_query, NULL);

    sql = "SELECT path FROM FILES WHERE id = ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getFilePath_query, NULL);

    sql = "SELECT timestamp FROM COMPILATION_UNIT WHERE id= ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getTimestampForFile_query, NULL);

    sql = "SELECT 1 FROM COMPILATION_UNIT WHERE id= ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &existsCompilationUnit_query, NULL);

    sql = "INSERT INTO `FILES`(`path`) VALUES (?1);";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &addFile_query, NULL);

    sql = "SELECT FILES.path, COMPILATION_UNIT.timestamp, COMPILATION_UNIT.cmd_args FROM COMPILATION_UNIT, FILES WHERE COMPILATION_UNIT.id=FILES.id;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getCompilationUnits_query, NULL);

    sql = "INSERT OR REPLACE INTO `LINES`(`file_id`,`line`,`original_line`) VALUES (?1, ?2, ?3);";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &updateLineOffsetMapping_query, NULL);

    sql = "UPDATE `LINES` SET line=?3, original_line=?4, file_id=?2; WHERE id=?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &updateLineById_query, NULL);

    sql = "SELECT REPLACEMENTS.id, REPLACEMENTS.file_id, REPLACEMENTS.start_line, REPLACEMENTS.lines_replaced, REPLACEMENTS.lines_in_replacement, "
        "REPLACEMENTS.transformation_id, REPLACEMENTS.comes_from_file_id, REPLACEMENTS.comes_from_start_line, REPLACEMENTS.comes_from_end_line, "
        " REPLACEMENTS.unhandled, REPLACEMENTS.replacement_text FROM REPLACEMENTS WHERE REPLACEMENTS.id= ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getReplacementById_query, NULL);

    sql = "SELECT id FROM LINES WHERE file_id= ?1 AND line= ?2;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getLineId_query, NULL);

    sql = "INSERT OR IGNORE INTO `LINES_REPLACEMENTS` (`line_id`,`replacement_id`) VALUES (?1, ?2);";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &lines_relations_query, NULL);

    sql = "INSERT OR IGNORE INTO `REPLACEMENTS` (`file_id`,`start_line`,`lines_replaced`,`lines_in_replacement`,`transformation_id`,`comes_from_file_id`,`comes_from_start_line`,`comes_from_end_line`,`replacement_text`) VALUES (?1, ?2, ?3, ?4, ?5, ?6, ?7, ?8, ?9);";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &addReplacement_insert_query, NULL);

    sql = "SELECT REPLACEMENTS.id FROM REPLACEMENTS WHERE file_id= ?1 AND start_line= ?2 AND lines_replaced= ?3 AND lines_in_replacement= ?4 AND transformation_id= ?5;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &addReplacement_select_query, NULL);

    sql = "SELECT LINES_REPLACEMENTS.replacement_id FROM LINES_REPLACEMENTS WHERE line_id= ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getReplacementsOfLine_query, NULL);

    sql = "INSERT OR IGNORE INTO `TRANSFORMATIONS` (`name`) VALUES (?1);";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getTransformationID_insert_query, NULL);

    sql = "SELECT TRANSFORMATIONS.id FROM TRANSFORMATIONS WHERE name= ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getTransformationID_select_query, NULL);

    sql = "SELECT TRANSFORMATIONS.name FROM TRANSFORMATIONS WHERE id= ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getTransformationName_query, NULL);

    sql = "SELECT LINES.original_line FROM LINES WHERE line= ?2 AND file_id = ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getOriginalLineNumber_query, NULL);

    sql = "SELECT LINES.line FROM LINES WHERE line_id= ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getActualLineNumberByLineId_query, NULL);

    sql = "SELECT REPLACEMENTS.id FROM REPLACEMENTS WHERE file_id= ?1 AND start_line<= ?2;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getReplacementsWithStartLineLessThanOrEqual_raw_query, NULL);

    sql = "SELECT REPLACEMENTS.id FROM REPLACEMENTS WHERE file_id= ?1 AND start_line<= ?2 AND transformation_id = ?3;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query, NULL);

    sql = "SELECT REPLACEMENTS.id FROM REPLACEMENTS WHERE unhandled <> 0";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getUnhandledReplacements_query, NULL);

    sql = "UPDATE REPLACEMENTS SET unhandled=0 WHERE REPLACEMENTS.id= ?1";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &setReplacementHandled_query, NULL);

    sql = "UPDATE FILES SET number_of_lines=?1 WHERE FILES.id= ?2";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &setTheNumberOfLinesForFile_query, NULL);

    sql = "SELECT FILES.number_of_lines FROM FILES WHERE FILES.id= ?1";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &getTheNumberOfLinesOfFile_query, NULL);

    sql = "SELECT LINES.id FROM LINES WHERE file_id= ?1 AND line= ?2;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &isActualLineInTheDb_query, NULL);

    sql = "DELETE FROM LINES WHERE file_id= ?1;";
    sqlite3_prepare_v2(database, sql.c_str(), -1, &resetLineForFile_query, NULL);

    return *this;
}

bool DatabaseConnection::createTables() {
    const char *filesTable = "CREATE TABLE IF NOT EXISTS `FILES` ( " \
        "`id`   INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"\
        "`number_of_lines` INTEGER,"\
        "`path` TEXT NOT NULL); " \
        "CREATE UNIQUE INDEX IF NOT EXISTS FILES_INDEX ON FILES (path);" \
        "CREATE UNIQUE INDEX IF NOT EXISTS FILES_INDEX_ID ON FILES (id);";

    const char *cuTable = "CREATE TABLE IF NOT EXISTS `COMPILATION_UNIT` ( " \
        "`id`   INTEGER NOT NULL PRIMARY KEY,"\
        "`timestamp`    DATETIME NOT NULL, " \
        "`cmd_args` TEXT NOT NULL, " \
        "FOREIGN KEY(`id`) REFERENCES FILES(id) ); " \
        "CREATE UNIQUE INDEX IF NOT EXISTS COMP_UNIT_INDEX ON COMPILATION_UNIT (id);";

    const char *relationsTable = "CREATE TABLE IF NOT EXISTS `RELATIONS` (" \
        "`file_id`  INTEGER NOT NULL," \
        "`dep_id`   INTEGER NOT NULL," \
        "`dependency_timestamp` DATETIME NOT NULL," \
        "PRIMARY KEY(file_id, dep_id), " \
        "FOREIGN KEY(`file_id`) REFERENCES FILES(id), " \
        "FOREIGN KEY(`dep_id`) REFERENCES FILES(id) ); " \
        "CREATE INDEX IF NOT EXISTS RELATIONS_INDEX ON RELATIONS (file_id, dep_id);";

    const char *transformationsTable = "CREATE TABLE IF NOT EXISTS `TRANSFORMATIONS` (" \
        "`id`  INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT," \
        "`name` TEXT NOT NULL);" \
        "CREATE UNIQUE INDEX IF NOT EXISTS TRANSFORMATIONS_INDEX_NAME ON TRANSFORMATIONS (name);" \
        "CREATE UNIQUE INDEX IF NOT EXISTS TRANSFORMATIONS_INDEX_ID ON TRANSFORMATIONS (id);";

    const char *replacementsTable = "CREATE TABLE IF NOT EXISTS `REPLACEMENTS` (" \
        "`id`  INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT," \
        "`file_id`   INTEGER NOT NULL," \
        "`start_line` INTEGER NOT NULL," \
        "`lines_replaced` INTEGER NOT NULL," \
        "`lines_in_replacement` INTEGER NOT NULL," \
        "`comes_from_start_line` INTEGER NOT NULL," \
        "`comes_from_end_line` INTEGER NOT NULL," \
        "`comes_from_file_id` INTEGER NOT NULL," \
        "`transformation_id` INTEGER NOT NULL," \
        "`unhandled` INTEGER DEFAULT(1)," \
        "`replacement_text` TEXT NOT NULL," \
        "FOREIGN KEY(`file_id`) REFERENCES FILES(id), " \
        "FOREIGN KEY(`comes_from_file_id`) REFERENCES FILES(id), " \
        "FOREIGN KEY(`transformation_id`) REFERENCES TRANSFORMATIONS(id) ); " \
        "CREATE UNIQUE INDEX IF NOT EXISTS REPLACEMENT_INDEX_4 ON REPLACEMENTS (id, file_id, start_line, lines_replaced, replacement_text);" \
        "CREATE UNIQUE INDEX IF NOT EXISTS REPLACEMENT_INDEX_1 ON REPLACEMENTS (id);" \
        "CREATE INDEX IF NOT EXISTS REPLACEMENT_INDEX_3 ON REPLACEMENTS (transformation_id);" \
        "CREATE INDEX IF NOT EXISTS REPLACEMENT_INDEX_2 ON REPLACEMENTS (file_id);";

    const char *linesTable = "CREATE TABLE IF NOT EXISTS `LINES` (" \
        "`id`   INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,"\
        "`file_id`  INTEGER NOT NULL," \

        "`line`  INTEGER NOT NULL," \
        "`original_line`  INTEGER NOT NULL," \
        "FOREIGN KEY(`file_id`) REFERENCES FILES(id) ); " \
        "CREATE UNIQUE INDEX IF NOT EXISTS LINES_INDEX_FILE_ID_LINE ON LINES (file_id, line);" \
        "CREATE UNIQUE INDEX IF NOT EXISTS LINES_INDEX_ID ON LINES (id);";

    const char *linesReplacementsTable = "CREATE TABLE IF NOT EXISTS `LINES_REPLACEMENTS` (" \
        "`line_id`  INTEGER NOT NULL," \
        "`replacement_id`  INTEGER NOT NULL," \
        "PRIMARY KEY(line_id, replacement_id), " \
        "FOREIGN KEY(`line_id`) REFERENCES LINES(id), " \
        "FOREIGN KEY(`replacement_id`) REFERENCES REPLACEMENTS(id) ); " \
        "CREATE UNIQUE INDEX IF NOT EXISTS LINES_REPLACEMENTS_INDEX_LINE_ID_REPLACEMENT_ID ON LINES_REPLACEMENTS (line_id, replacement_id);" \
        "CREATE INDEX IF NOT EXISTS LINES_REPLACEMENTS_INDEX_REPLACEMENT_ID ON LINES_REPLACEMENTS (replacement_id);" \
        "CREATE INDEX IF NOT EXISTS LINES_REPLACEMENTS_INDEX_ID ON LINES_REPLACEMENTS (line_id);";


    const char* tableDefinitions[] = { filesTable, cuTable, relationsTable, transformationsTable,
        replacementsTable, linesTable, linesReplacementsTable };

    for (const char* tableDef : tableDefinitions) {
        if (!runQuery(tableDef, "Table created successfully"))
            return false;
    }

    return true;
}

bool DatabaseConnection::removeTables() {
    const char* sql = "DROP TABLE IF EXISTS `FILES`; DROP TABLE IF EXISTS `COMPILATION_UNIT`; DROP TABLE IF EXISTS `RELATIONS`;"
        " DROP INDEX IF EXISTS `FILES_INDEX`; DROP INDEX IF EXISTS `COMP_UNIT_INDEX`; DROP INDEX IF EXISTS `RELATIONS_INDEX`; DROP INDEX IF EXISTS `FILES_INDEX_ID`;"

        "DROP TABLE IF EXISTS `TRANSFORMATIONS`; DROP TABLE IF EXISTS `REPLACEMENTS`; DROP TABLE IF EXISTS `LINES`;  DROP TABLE IF EXISTS `LINES_REPLACEMENTS`;"
        " DROP INDEX IF EXISTS `TRANSFORMATIONS_INDEX_NAME`; DROP INDEX IF EXISTS `TRANSFORMATIONS_INDEX_ID`; DROP INDEX IF EXISTS `REPLACEMENT_INDEX_1`;"
        " DROP INDEX IF EXISTS `REPLACEMENT_INDEX_3`; DROP INDEX IF EXISTS `REPLACEMENT_INDEX_2`; DROP INDEX IF EXISTS `LINES_INDEX_FILE_ID_LINE`;"
        " DROP INDEX IF EXISTS `LINES_INDEX_ID`; DROP INDEX IF EXISTS `LINES_REPLACEMENTS_INDEX_LINE_ID_REPLACEMENT_ID`; DROP INDEX IF EXISTS `LINES_REPLACEMENTS_INDEX_REPLACEMENT_ID`;"
        " DROP INDEX IF EXISTS `LINES_REPLACEMENTS_INDEX_ID`;";
    return runQuery(sql, "Tables removed");
}

char* DatabaseConnection::beginTransaction() {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);
    char* sErrMsg = nullptr;
    sqlite3_exec(database, "BEGIN TRANSACTION", NULL, NULL, &sErrMsg);
    return sErrMsg;
}

char* DatabaseConnection::endTransaction() {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);
    char* sErrMsg = nullptr;
    sqlite3_exec(database, "END TRANSACTION", NULL, NULL, &sErrMsg);
    return sErrMsg;
}

bool DatabaseConnection::isFileInDb(const std::string& filePath) {
    return getIdForFile(filePath) != -1;
}

std::vector<backport::FileDatas> DatabaseConnection::getCompilationUnits() {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    std::vector<backport::FileDatas> ret;
    while (sqlite3_step(getCompilationUnits_query) == SQLITE_ROW) {
        std::string path = (char*)sqlite3_column_text(getCompilationUnits_query, 0);
        time_t modified = (time_t)sqlite3_column_int64(getCompilationUnits_query, 1);
        std::string args = (char*)sqlite3_column_text(getCompilationUnits_query, 2);
        ret.push_back({ path, modified, args });
    }
    sqlite3_reset(getCompilationUnits_query);
    return ret;
}

std::vector<backport::FileDatas> DatabaseConnection::getDependenciesList(const std::string& filePath) {
    return getDependenciesList(getIdForFile(filePath));
}

std::vector<backport::FileDatas> DatabaseConnection::getDependenciesList(int id) {
    std::vector<backport::FileDatas> dependencies;

    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getDependencyList_query, 1, id);

    while (sqlite3_step(getDependencyList_query) == SQLITE_ROW) {
        char* path = (char*)sqlite3_column_text(getDependencyList_query, 0);
        time_t modified = (time_t)sqlite3_column_int64(getDependencyList_query, 1);
        dependencies.push_back({ path, modified });
    }

    sqlite3_clear_bindings(getDependencyList_query);
    sqlite3_reset(getDependencyList_query);

    return dependencies;

}

bool DatabaseConnection::updateRelation(const std::string& filePath, const std::string& dependencyPath, time_t date) {
    return updateRelation(getIdForFile(filePath), getIdForFile(dependencyPath), date);
}

bool DatabaseConnection::updateRelation(int id_t1, int id_t2, time_t date) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int64(updateRelation_query, 1, date);
    sqlite3_bind_int(updateRelation_query, 2, id_t1);
    sqlite3_bind_int(updateRelation_query, 3, id_t2);

    bool res = runQuery(updateRelation_query, "Update successful");

    sqlite3_clear_bindings(updateRelation_query);
    sqlite3_reset(updateRelation_query);

    return res;
}

bool DatabaseConnection::updateTimestamp(int id, time_t date) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int64(updateCompUnitTimeStamp_query, 1, date);
    sqlite3_bind_int(updateCompUnitTimeStamp_query, 2, id);

    bool res = runQuery(updateCompUnitTimeStamp_query, "Update successful");

    sqlite3_clear_bindings(updateCompUnitTimeStamp_query);
    sqlite3_reset(updateCompUnitTimeStamp_query);

    return res;
}

bool DatabaseConnection::updateTimestamp(const std::string& path, time_t d) {
    return updateTimestamp(getIdForFile(path), d);
}

bool DatabaseConnection::updateCmdArguments(int id, const std::string& arg) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_text(updateCmdArgs_query, 1, arg.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(updateCmdArgs_query, 2, id);

    bool res = runQuery(updateCmdArgs_query, "Update successful");

    sqlite3_clear_bindings(updateCmdArgs_query);
    sqlite3_reset(updateCmdArgs_query);

    return res;
}

bool DatabaseConnection::updateCmdArguments(const std::string& path, const std::string& args) {
    return updateCmdArguments(getIdForFile(path), args);
}

bool DatabaseConnection::addCompilationUnit(int id, time_t d, const std::string& args) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(addCompUnit_query, 1, id);
    sqlite3_bind_int64(addCompUnit_query, 2, d);
    sqlite3_bind_text(addCompUnit_query, 3, args.c_str(), -1, SQLITE_TRANSIENT);

    bool res = runQuery(addCompUnit_query, "Insertion successful");

    sqlite3_clear_bindings(addCompUnit_query);
    sqlite3_reset(addCompUnit_query);

    return res;
}

bool DatabaseConnection::addCompilationUnit(const std::string& path, time_t d, const std::string& args) {
    return addCompilationUnit(getIdForFile(path), d, args);
}

bool DatabaseConnection::addRelation(int f_id, int d_id, time_t d) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(addRelation_query, 1, f_id);
    sqlite3_bind_int(addRelation_query, 2, d_id);
    sqlite3_bind_int64(addRelation_query, 3, d);

    bool res = runQuery(addRelation_query, "Insertion successful");

    sqlite3_clear_bindings(addRelation_query);
    sqlite3_reset(addRelation_query);

    return res;
}

bool DatabaseConnection::addRelation(const std::string& file_path, const std::string& dep_path, time_t d) {
    return addRelation(getIdForFile(file_path), getIdForFile(dep_path), d);
}

bool DatabaseConnection::removeAllDependencies(int id) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(removeAllDependency_query, 1, id);

    bool res = runQuery(removeAllDependency_query, "Delete successful");

    sqlite3_clear_bindings(removeAllDependency_query);
    sqlite3_reset(removeAllDependency_query);

    return res;
}

bool DatabaseConnection::removeAllDependencies(const std::string& filePath) {
    return removeAllDependencies(getIdForFile(filePath));
}

bool DatabaseConnection::runRawQuery(const std::string& query) {
    return runQuery(query.c_str(), "Query succesfull!");
}


std::string DatabaseConnection::getCmdLineForFile(const std::string& filePath) {
    return getCmdLineForFile(getIdForFile(filePath));
}

std::string DatabaseConnection::getCmdLineForFile(int id) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    std::string ret;

    sqlite3_bind_int(getCmdLineForFile_query, 1, id);

    while (sqlite3_step(getCmdLineForFile_query) == SQLITE_ROW) {
        ret = (char*)sqlite3_column_text(getCmdLineForFile_query, 0);
    }

    sqlite3_clear_bindings(getCmdLineForFile_query);
    sqlite3_reset(getCmdLineForFile_query);

    return ret;
}



time_t DatabaseConnection::getTimestampForFile(const std::string& filePath) {
    return getTimestampForFile(getIdForFile(filePath));
}

time_t DatabaseConnection::getTimestampForFile(int id) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    time_t time = -1;

    sqlite3_bind_int(getTimestampForFile_query, 1, id);

    while (sqlite3_step(getTimestampForFile_query) == SQLITE_ROW) {
        time = (time_t)sqlite3_column_int64(getTimestampForFile_query, 0);
    }

    sqlite3_clear_bindings(getTimestampForFile_query);
    sqlite3_reset(getTimestampForFile_query);

    return time;
}

bool DatabaseConnection::isCompilationUnitInDb(int id) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    bool found = false;

    sqlite3_bind_int(existsCompilationUnit_query, 1, id);
    while (sqlite3_step(existsCompilationUnit_query) == SQLITE_ROW) {
        found = true;
    }

    sqlite3_clear_bindings(existsCompilationUnit_query);
    sqlite3_reset(existsCompilationUnit_query);

    return found;
}


bool DatabaseConnection::isCompilationUnitInDb(const std::string& filePath) {
    return isCompilationUnitInDb(getIdForFile(filePath));
}

int DatabaseConnection::getIdForFile(const std::string& filepath) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    int id = -1;
    sqlite3_bind_text(getIdForFile_query, 1, filepath.c_str(), -1, SQLITE_TRANSIENT);
    while (sqlite3_step(getIdForFile_query) == SQLITE_ROW) {
        id = sqlite3_column_int(getIdForFile_query, 0);
    }

    sqlite3_clear_bindings(getIdForFile_query);
    sqlite3_reset(getIdForFile_query);

    return id;
}

std::string DatabaseConnection::getFilePath(int file_id){
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getFilePath_query, 1, file_id);

    std::string path = "";
    while (sqlite3_step(getFilePath_query) == SQLITE_ROW) {
        // TODO: UNICODE PATHS :??? :(((
        path = std::string(reinterpret_cast<char const *>(sqlite3_column_text(getFilePath_query, 0)));
    }

    sqlite3_clear_bindings(getFilePath_query);
    sqlite3_reset(getFilePath_query);

    return path;
}

bool DatabaseConnection::runQuery(const char* query, const char* type) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    bool success = true;
    char *zErrMsg = 0;

    const int rc = sqlite3_exec(database, query, callback, 0, &zErrMsg);
        
    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        LOG(logERROR) << "----\n" << "SQL error: " << zErrMsg << "\nquery was :\n" << query << "\n---";
        sqlite3_free(zErrMsg);
        success = false;
    }

    return success;
}

bool DatabaseConnection::runQuery(sqlite3_stmt *stmt, const char* type, std::set<int> const& ignoreError) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    bool success = true;

    const int rc = sqlite3_step(stmt);
        
    if (rc != SQLITE_OK && rc != SQLITE_DONE) {
        if (ignoreError.count(rc)) {
            LOG(logDEBUG) << "The following error was ignored:\n----\nSQL error: " << sqlite3_errmsg(database) << "\nError code was: " << sqlite3_errcode(database) << "\nquery was:\n" << sqlite3_sql(stmt) << "\n---";
        } else {
            LOG(logERROR) << "----\nSQL error: " << sqlite3_errmsg(database) << "\nError code was: " << sqlite3_errcode(database) << "\nquery was:\n" << sqlite3_sql(stmt) << "\n---";
            success = false;
        }
    }

    return success;
}

bool DatabaseConnection::addFile(const std::string& path) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_text(addFile_query, 1, path.c_str(), path.size(), SQLITE_TRANSIENT);

    bool res = runQuery(addFile_query, "Records created successfully");

    sqlite3_clear_bindings(addFile_query);
    sqlite3_reset(addFile_query);

    return res;
}

bool DatabaseConnection::updateLineOffsetMapping(int file_id, int actualLine, int originalLine) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(updateLineOffsetMapping_query, 1, file_id);
    sqlite3_bind_int(updateLineOffsetMapping_query, 2, actualLine);
    sqlite3_bind_int(updateLineOffsetMapping_query, 3, originalLine);

    bool res = runQuery(updateLineOffsetMapping_query, "Line, originalLine relation updated/inserted successfully.");

    sqlite3_clear_bindings(updateLineOffsetMapping_query);
    sqlite3_reset(updateLineOffsetMapping_query);

    return res;
}

bool DatabaseConnection::updateLineOffsetMapping(std::string const &filepath, int actualLine, int originalLine) {
    return updateLineOffsetMapping(getIdForFile(filepath), actualLine, originalLine);
}

backport::ReplacementData DatabaseConnection::getReplacementById(long replacementid) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getReplacementById_query, 1, replacementid);

    backport::ReplacementData result;
    while (sqlite3_step(getReplacementById_query) == SQLITE_ROW) {

        std::string filepath;
        int file_id;

        long replacementId;
        int startLine;
        int numberOfLinesReplaced;
        int numberOfLinesInReplacementText;
        short transformationId;
        int from_file_id;
        int from_start_line;
        int from_end_line;
        bool unhandled;
        std::string replacement_text;

        replacementId = sqlite3_column_int(getReplacementById_query, 0);
        file_id = sqlite3_column_int(getReplacementById_query, 1);
        startLine = sqlite3_column_int(getReplacementById_query, 2);
        numberOfLinesReplaced = sqlite3_column_int(getReplacementById_query, 3);
        numberOfLinesInReplacementText = sqlite3_column_int(getReplacementById_query, 4);
        transformationId = sqlite3_column_int(getReplacementById_query, 5);
        from_file_id = sqlite3_column_int(getReplacementById_query, 6);
        from_start_line = sqlite3_column_int(getReplacementById_query, 7);
        from_end_line = sqlite3_column_int(getReplacementById_query, 8);
        unhandled = sqlite3_column_int(getReplacementById_query, 9) != 0;
        replacement_text = std::string(reinterpret_cast<char const *>(sqlite3_column_text(getReplacementById_query, 10)));

        filepath = getFilePath(file_id);

        result = backport::ReplacementData(transformationId, filepath, file_id, replacementId, startLine, numberOfLinesReplaced,
            numberOfLinesInReplacementText, from_file_id, getFilePath(from_file_id), from_start_line, from_end_line, unhandled, replacement_text);
    }

    sqlite3_clear_bindings(getReplacementById_query);
    sqlite3_reset(getReplacementById_query);

    return result;
}

int DatabaseConnection::getLineId(int file_id , int line) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getLineId_query, 1, file_id);
    sqlite3_bind_int(getLineId_query, 2, line);

    int result = -1;
    while (sqlite3_step(getLineId_query) == SQLITE_ROW) {
        result = sqlite3_column_int(getLineId_query, 0);
    }

    sqlite3_clear_bindings(getLineId_query);
    sqlite3_reset(getLineId_query);

    return result;
}

int DatabaseConnection::getLineId(std::string const& filePath, int line) {
    return getLineId(getIdForFile(filePath), line);
}

bool DatabaseConnection::updateLineOffsetMapping(int file_id, long replacementId, int actualLine, int originalLine) {
    bool res = updateLineOffsetMapping(file_id, actualLine, originalLine);

    if (res == false)
        return false;

    int line_id = getLineId(file_id, actualLine);

    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(lines_relations_query, 1, getLineId(file_id, actualLine));
    sqlite3_bind_int(lines_relations_query, 2, replacementId);

    res = runQuery(lines_relations_query, "The replacement has been attached to the line successfully.");

    sqlite3_clear_bindings(lines_relations_query);
    sqlite3_reset(lines_relations_query);

    return res;
}

bool DatabaseConnection::updateLineOffsetMapping(std::string const &filepath, long replacementId, int actualLine, int originalLine) {
    return updateLineOffsetMapping(getIdForFile(filepath), replacementId, actualLine, originalLine);
}

long DatabaseConnection::addReplacement(int file_id, short transformationId, int startLine, int numberOfLinesReplaced,
    int numberOfLinesInReplacementText, int from_file_id, int from_start_line, int from_end_line, std::string const &replacement_text) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(addReplacement_insert_query, 1, file_id);
    sqlite3_bind_int(addReplacement_insert_query, 2, startLine);
    sqlite3_bind_int(addReplacement_insert_query, 3, numberOfLinesReplaced);
    sqlite3_bind_int(addReplacement_insert_query, 4, numberOfLinesInReplacementText);
    sqlite3_bind_int(addReplacement_insert_query, 5, transformationId);

    sqlite3_bind_int(addReplacement_insert_query, 6, from_file_id);
    sqlite3_bind_int(addReplacement_insert_query, 7, from_start_line);
    sqlite3_bind_int(addReplacement_insert_query, 8, from_end_line);

    sqlite3_bind_text(addReplacement_insert_query, 9, replacement_text.c_str(), -1, SQLITE_TRANSIENT);

    bool res = runQuery(addReplacement_insert_query, "The replacement has been successfully registered.");

    sqlite3_clear_bindings(addReplacement_insert_query);
    sqlite3_reset(addReplacement_insert_query);

    if (res == false)
        return -1;
    
    sqlite3_bind_int(addReplacement_select_query, 1, file_id);
    sqlite3_bind_int(addReplacement_select_query, 2, startLine);
    sqlite3_bind_int(addReplacement_select_query, 3, numberOfLinesReplaced);
    sqlite3_bind_int(addReplacement_select_query, 4, numberOfLinesInReplacementText);
    sqlite3_bind_int(addReplacement_select_query, 5, transformationId);

    long result = -1;

    while (sqlite3_step(addReplacement_select_query) == SQLITE_ROW) {
        result = sqlite3_column_int64(addReplacement_select_query, 0);
    }

    sqlite3_clear_bindings(addReplacement_select_query);
    sqlite3_reset(addReplacement_select_query);

    return result;
}

long DatabaseConnection::addReplacement(std::string const &filepath, short transformationId, int startLine, int numberOfLinesReplaced,
    int numberOfLinesInReplacementText, std::string const &fromPath,
    int from_start_line, int from_end_line, std::string const &replacement_text) {
    return addReplacement(getIdForFile(filepath), transformationId, startLine, numberOfLinesReplaced, numberOfLinesInReplacementText,
        getIdForFile(fromPath), from_start_line, from_end_line, replacement_text);
}

std::vector<backport::ReplacementData> DatabaseConnection::getReplacementsOfLine(int file_id, int line) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    int line_id = getLineId(file_id, line);

    std::vector<backport::ReplacementData> result;

    sqlite3_bind_int(getReplacementsOfLine_query, 1, line);

    while (sqlite3_step(getReplacementsOfLine_query) == SQLITE_ROW) {
        int replacement_id = sqlite3_column_int(getReplacementsOfLine_query, 0);
        result.push_back(getReplacementById(replacement_id));
    }

    sqlite3_clear_bindings(getReplacementsOfLine_query);
    sqlite3_reset(getReplacementsOfLine_query);

    return result;

}

std::vector<backport::ReplacementData> DatabaseConnection::getReplacementsOfLine(std::string const& filePath, int line) {
    return getReplacementsOfLine(getIdForFile(filePath), line);
}

std::set<short> DatabaseConnection::getTransformationIdsOfLine(int file_id, int line) {
    auto replacements = getReplacementsOfLine(file_id, line);

    std::set<short> result;

    for (auto &c : replacements) {
        result.insert(c.transformationId);
    }

    return result;
}

std::set<short> DatabaseConnection::getTransformationIdsOfLine(std::string const& filePath, int line) {
    return getTransformationIdsOfLine(getIdForFile(filePath), line);
}

short DatabaseConnection::getTransformationID(std::string const &transformationName) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_text(getTransformationID_insert_query, 1, transformationName.c_str(), -1, SQLITE_TRANSIENT);

    bool res = runQuery(getTransformationID_insert_query, "Transformation name inserted, or if already inserted: nop.");

    if (res == false)
        return -1;

    sqlite3_clear_bindings(getTransformationID_insert_query);
    sqlite3_reset(getTransformationID_insert_query);

    sqlite3_bind_text(getTransformationID_select_query, 1, transformationName.c_str(), -1, SQLITE_TRANSIENT);

    short result = -1;

    while (sqlite3_step(getTransformationID_select_query) == SQLITE_ROW) {
        result = sqlite3_column_int(getTransformationID_select_query, 0);
    }

    sqlite3_clear_bindings(getTransformationID_select_query);
    sqlite3_reset(getTransformationID_select_query);


    return result;
}

std::string DatabaseConnection::getTransformationName(short transformationId) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getTransformationName_query, 1, transformationId);

    std::string result = "";

    while (sqlite3_step(getTransformationName_query) == SQLITE_ROW) {
        result = std::string(reinterpret_cast<char const *>(sqlite3_column_text(getTransformationName_query, 0)));
    }

    sqlite3_clear_bindings(getTransformationName_query);
    sqlite3_reset(getTransformationName_query);

    return result;
}

int DatabaseConnection::getOriginalLineNumber(int file_id, int actualLine) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getOriginalLineNumber_query, 1, file_id);
    sqlite3_bind_int(getOriginalLineNumber_query, 2, actualLine);

    int result = -1000;
    while (sqlite3_step(getOriginalLineNumber_query) == SQLITE_ROW) {
        result = sqlite3_column_int(getOriginalLineNumber_query, 0);
    }

    sqlite3_clear_bindings(getOriginalLineNumber_query);
    sqlite3_reset(getOriginalLineNumber_query);

    if (result == -1000) {
        return actualLine;
    }
    else {
        return result;
    }
}


int DatabaseConnection::getOriginalLineNumber(std::string const &filepath, int actualLine) {
    return getOriginalLineNumber(getIdForFile(filepath), actualLine);
}

bool DatabaseConnection::updateLineNumberById(int file_id, int lineId, int newLineNumber, int originalLineNumber) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(updateLineById_query, 1, lineId);
    sqlite3_bind_int(updateLineById_query, 2, file_id);
    sqlite3_bind_int(updateLineById_query, 3, newLineNumber);
    sqlite3_bind_int(updateLineById_query, 4, originalLineNumber);

    bool res = runQuery(updateLineById_query, "Updating line offset was successfull.");

    sqlite3_clear_bindings(updateLineById_query);
    sqlite3_reset(updateLineById_query);

    return res;
}

bool DatabaseConnection::updateLineNumberById(std::string const &filepath, int lineId, int newLineNumber, int originalLineNumber) {
    return updateLineNumberById(getIdForFile(filepath), lineId, newLineNumber, originalLineNumber);
}

int DatabaseConnection::getActualLineNumberByLineId(int lineId) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getActualLineNumberByLineId_query, 1, lineId);

    int result = -1;
    while (sqlite3_step(getActualLineNumberByLineId_query) == SQLITE_ROW) {
        result = sqlite3_column_int(getActualLineNumberByLineId_query, 0);
    }

    sqlite3_clear_bindings(getActualLineNumberByLineId_query);
    sqlite3_reset(getActualLineNumberByLineId_query);

    return result;
}

std::vector<backport::ReplacementData> DatabaseConnection::getReplacementsWithStartLineLessThanOrEqual_raw(int file_id, int line) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);
    
    sqlite3_bind_int(getReplacementsWithStartLineLessThanOrEqual_raw_query, 1, file_id);
    sqlite3_bind_int(getReplacementsWithStartLineLessThanOrEqual_raw_query, 2, line);

    std::vector<backport::ReplacementData> result;

    while (sqlite3_step(getReplacementsWithStartLineLessThanOrEqual_raw_query) == SQLITE_ROW) {
        long tmp = sqlite3_column_int64(getReplacementsWithStartLineLessThanOrEqual_raw_query, 0);
        result.push_back(getReplacementById(tmp));
    }

    sqlite3_clear_bindings(getReplacementsWithStartLineLessThanOrEqual_raw_query);
    sqlite3_reset(getReplacementsWithStartLineLessThanOrEqual_raw_query);

    return result;

}
std::vector<backport::ReplacementData> DatabaseConnection::getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw(int file_id, short transformationId, int line){
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query, 1, file_id);
    sqlite3_bind_int(getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query, 2, line);
    sqlite3_bind_int(getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query, 3, transformationId);

    std::vector<backport::ReplacementData> result;

    while (sqlite3_step(getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query) == SQLITE_ROW) {
        long tmp = sqlite3_column_int64(getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query, 0);
        result.push_back(getReplacementById(tmp));
    }

    sqlite3_clear_bindings(getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query);
    sqlite3_reset(getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw_query);

    return result;
}

std::vector<backport::ReplacementData> DatabaseConnection::getReplacementsWithStartLineLessThanOrEqual_raw(std::string const &filePath, int line) {
    return getReplacementsWithStartLineLessThanOrEqual_raw(getIdForFile(filePath), line);
}
std::vector<backport::ReplacementData> DatabaseConnection::getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw(std::string const &filePath, short transformationId, int line){
    return getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw(getIdForFile(filePath), transformationId, line);
}

std::vector<backport::ReplacementData> DatabaseConnection::getUnhandledReplacements() {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    std::vector<backport::ReplacementData> result;

    while (sqlite3_step(getUnhandledReplacements_query) == SQLITE_ROW) {
        long tmp = sqlite3_column_int64(getUnhandledReplacements_query, 0);
        result.push_back(getReplacementById(tmp));
    }

    return result;
}

bool DatabaseConnection::setReplacementHandled(long replacementID) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);
    
    sqlite3_bind_int(setReplacementHandled_query, 1, replacementID);

    bool res = runQuery(setReplacementHandled_query, "Replacement marked as traced.");

    sqlite3_clear_bindings(setReplacementHandled_query);
    sqlite3_reset(setReplacementHandled_query);

    return res;
}

bool DatabaseConnection::setTheNumberOfLinesForFile(int file_id, int lines) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(setTheNumberOfLinesForFile_query, 1, lines);
    sqlite3_bind_int(setTheNumberOfLinesForFile_query, 2, file_id);

    bool res = runQuery(setTheNumberOfLinesForFile_query, "Line number for the file has been set.");

    sqlite3_clear_bindings(setTheNumberOfLinesForFile_query);
    sqlite3_reset(setTheNumberOfLinesForFile_query);

    return res;

}
int DatabaseConnection::getTheNumberOfLinesOfFile(int file_id) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(getTheNumberOfLinesOfFile_query, 1, file_id);

    int result = -1;
    
    while (sqlite3_step(getTheNumberOfLinesOfFile_query) == SQLITE_ROW) {
        result = sqlite3_column_int(getTheNumberOfLinesOfFile_query, 0);
    }

    sqlite3_clear_bindings(getTheNumberOfLinesOfFile_query);
    sqlite3_reset(getTheNumberOfLinesOfFile_query);

    return result;

}

bool DatabaseConnection::setTheNumberOfLinesForFile(std::string const &filePath, int lines) {
    return setTheNumberOfLinesForFile(getIdForFile(filePath), lines);
}
int DatabaseConnection::getTheNumberOfLinesOfFile(std::string const &filePath) {
    return getTheNumberOfLinesOfFile(getIdForFile(filePath));
}

bool DatabaseConnection::isActualLineInTheDb(int file_id, int line) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(isActualLineInTheDb_query, 1, file_id);
    sqlite3_bind_int(isActualLineInTheDb_query, 2, line);

    bool res = false;

    while (sqlite3_step(isActualLineInTheDb_query) == SQLITE_ROW) {
        res = true;
        break;
    }

    sqlite3_clear_bindings(isActualLineInTheDb_query);
    sqlite3_reset(isActualLineInTheDb_query);


    return res;
}

bool DatabaseConnection::isActualLineInTheDb(std::string const &filePath, int line) {
    return isActualLineInTheDb(getIdForFile(filePath), line);
}


bool DatabaseConnection::resetLineForFile(std::string const &filepath) {
    return resetLineForFile(getIdForFile(filepath));
}


bool DatabaseConnection::resetLineForFile(int file_id) {
    std::lock_guard<std::recursive_mutex> lock(operationIsInProgressMutex);

    sqlite3_bind_int(resetLineForFile_query, 1, file_id);

    bool res = runQuery(resetLineForFile_query, "Lines deleted for the requested file");


    sqlite3_clear_bindings(resetLineForFile_query);
    sqlite3_reset(resetLineForFile_query);

    return res;
}
