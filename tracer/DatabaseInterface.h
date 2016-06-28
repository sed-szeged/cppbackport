#ifndef _DBINTERFACE_HEADER_
#define _DBINTERFACE_HEADER_

#include <stdio.h>
#include "sqlite/sqlite3.h"
#include <iostream>
#include <ctime>
#include "Util/FileDatas.h"
#include "Util/ReplacementData.h"
#include <set>
#include <vector>

class DatabaseInterface {
public:

    virtual ~DatabaseInterface() {};

    /**
     * Database initialization
     */
    virtual void initDb() = 0;

    /**
     * Create tables (if not exists) in the opened database
     */
    virtual bool createTables() = 0;

    /**
     * Truncate the current database
     */
    virtual bool truncateDatabase() = 0;


    /**
     * Adds a file to the Files table
     * \param path the absolute path of the file
     */
    virtual bool addFile(const std::string& path) = 0;

    /**
     * Adds a compilation unit to the compilation unit table
     * \param path the absolute path of the file
     * \param d the compilations unit's timestamp
     * \param args the compilation unit's command line arguments
     */
    virtual bool addCompilationUnit(const std::string& path, time_t d, const std::string& args) = 0;

    /**
     * Adds a relation to the relation's table
     * \param filePath the absolute path of the file
     * \param depPath the absolute path of the dependency file
     * \param dependencyTimeStamp the dependency file's timestamp
     */
    virtual bool addRelation(const std::string& filePath, const std::string& depPath, time_t dependencyTimeStamp) = 0;

    /**
     * Updates relation timestamp between filePath and depPath 
     * \param filePath the absolute path of the file
     * \param depPath the absolute path of the dependency file
     * \param dependencyTimeStamp the dependency file's timestamp
     */
    virtual bool updateRelation(const std::string& filePath, const std::string& depPath, time_t dependencyTimeStamp) = 0;

    /**
     * Updates file timestamp
     * \param path the absolute path of the file
     * \param date the dependency file's timestamp
     */
    virtual bool updateTimestamp(const std::string& path, time_t date) = 0;

    /**
     * Returns all stored compilations units.
     */
    virtual std::vector<backport::FileDatas> getCompilationUnits() = 0;

    /**
     * Returns with the command line arguments for a file
     * \param filePath the absolute path of the file
     */
    virtual std::string getCmdLineForFile(const std::string& filePath) = 0;

    /**
     * Returns with file's timestamp
     * \param filePath the absolute path of the file
     */
    virtual time_t getTimestampForFile(const std::string& filePath) = 0;

    /**
     * Returns with the dependencies list of a file
     * \param filePath the absolute path of the file
     */
    virtual std::vector<backport::FileDatas> getDependenciesList(const std::string& filePath) = 0;

    /**
     * Returns true, if the file is in the database
     * \param filePath the absolute path of the file
     */
    virtual bool isFileInDb(const std::string& filePath) = 0;

    /**
     * Returns true, if the compilation unit is in the database
     * \param filePath the absolute path of the file
     */
    virtual bool isCompilationUnitInDb(const std::string& filePath) = 0;

    /**
     * Removes all the dependencies
     * \param filePath the absolute path of the file
     */
    virtual bool removeAllDependencies(const std::string& filePath) = 0;

    /**
     * Returns true, if the file is in the database
     * \param filePath the absolute path of the file
     * \param args the new arguments
     */
    virtual bool updateCmdArguments(const std::string& path, const std::string& arg) = 0;

    /**
     * Starts a new transaction.
     */
    virtual char* beginTransaction() = 0;

    /**
    * Finishes a transaction.
    */
    virtual char* endTransaction() = 0;

    /**
    * Insert or Update the line => original line mapping.
    *
    * This is must be used just to update offset.
    * Only use this if the line weren't changed by a transformation only the line number changed bc
    * the transformation before this line changed the number of lines before this line.
    */
    virtual bool updateLineOffsetMapping(std::string const &filepath, int actualLine, int originalLine) = 0;

    /**
    * Offsets a line. 
    */
    virtual bool updateLineNumberById(std::string const &filepath, int lineId, int newLineNumber, int originalLineNumber) = 0;


    /**
    * Insert or Update the line => original line mapping.
    *
    * This is must be used to update the mapping if these lines were affected by the transformation.
    */
    virtual bool updateLineOffsetMapping(std::string const &filepath, long replacementId, int actualLine, int originalLine) = 0;

    /**
    * Register a transformation's replacement.
    */
    virtual long addReplacement(std::string const &filepath, short transformationId, int startLine, int numberOfLinesReplaced, int numberOfLinesInReplacementText, std::string const &fromPath, int from_start_line, int from_end_line, std::string const &replacement_text) = 0;

    /**
    * Gives a list of replacements that modified that line.
    */
    virtual std::vector<backport::ReplacementData> getReplacementsOfLine(std::string const& filePath, int line) = 0;

    /**
    * Gives the details of a replacement from the replacement id.
    */
    virtual backport::ReplacementData getReplacementById(long replacementid) = 0;

    /**
    * Gives a list of transformationIds that modified that line.
    */
    virtual std::set<short> getTransformationIdsOfLine(std::string const& filePath, int line) = 0;

    /**
    * Gives a list of transformationIds that modified that line.
    */
    virtual int getLineId(std::string const& filePath, int line) = 0;

    /**
    * Get the Id of a Transformation.
    */
    virtual short getTransformationID(std::string const &transformationName) = 0;

    /**
    * Get the name of a Transformation.
    */
    virtual std::string getTransformationName(short transformationId) = 0;

    /**
    * Get the original line number of a line number.
    */
    virtual int getOriginalLineNumber(std::string const &filepath, int actualLine) = 0;

    /**
    * Get the actual (after transformation) line number of a line from line_id.
    */
    virtual int getActualLineNumberByLineId(int lineId) = 0;

    virtual std::vector<backport::ReplacementData> getReplacementsWithStartLineLessThanOrEqual_raw(std::string const &filePath, int line) = 0;
    virtual std::vector<backport::ReplacementData> getReplacementsWithStartLineLessThanOrEqualOfTransformationId_raw(std::string const &filePath, short transformationId, int line) = 0;
    virtual std::vector<backport::ReplacementData> getUnhandledReplacements() = 0;
    virtual bool setReplacementHandled(long replacementID) = 0;

    virtual bool setTheNumberOfLinesForFile(std::string const &filePath, int lines) = 0;
    virtual int getTheNumberOfLinesOfFile(std::string const &filePath) = 0;
    virtual bool isActualLineInTheDb(std::string const &filePath, int line) = 0;
};

#endif
