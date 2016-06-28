#include "BackportManager.h"
#include "IncludeAction.h"
#include "TransformBase/Tooling.h"
#include <algorithm>
#include <sstream>
#include <set>
#include "Util/Log.h"
#include "Util/Timer.h"

using namespace clang;
using namespace clang::tooling;
using namespace backport;

BackportManager::BackportManager(std::vector<backport::helper::Path> sources, const std::string& dbFile, bool useDb, bool resetDb, bool child) : sources(sources), useDatabase(useDb) {
    dbc = std::make_shared<DatabaseConnection>(dbFile);
    if (useDb) {
        dbc->initDb();
        if (!child) {
            if (resetDb) {
                dbc->truncateDatabase();
            }
            loadDependencies();
        }
    }
}

BackportManager::~BackportManager() {
    // shared_ptr will delete the dbc automaticly.
    // The virtual destructor of it will close the connection too.
}

void BackportManager::loadDependencies() {
    for (auto elem : dbc->getCompilationUnits()) {
        auto ret = dbc->getDependenciesList(elem.path);
        if (!ret.size()) {
            //We want to keep files which does not have any include
            dependencies[elem] = std::set<FileDatas>();
        }
        else {
            LOG(logDEBUG) << "Loading dependencies for: " << elem.path << " Number of dependencies: " << ret.size();
            dependencies[elem] = std::set<FileDatas>(ret.begin(), ret.end());
        }
    }
}

void BackportManager::makeListForTransformation() {
    if (dependencies.size() == 0)
        return;

    LOG(logINFO) << "Collecting files for transformation...";

    for (auto currentFile = dependencies.begin(), currentEnd = dependencies.end(); currentFile != currentEnd; ++currentFile) {

        // Compilation unit
        FileDatas file = currentFile->first;
        bool isFileInDb = dbc->isFileInDb(file.path);

        // File not in database or command line arguments not equal.
        if (!isFileInDb || file.cmdLine != dbc->getCmdLineForFile(file.path)) {
            if (!isFileInDb) {
                LOG(logDEBUG) << "File is not in the database... File: " << file.path;
            }
            else {
                LOG(logDEBUG) << "Command line arguments changed for file: " << file.path;
                LOG(logDEBUG) << file.cmdLine;
                LOG(logDEBUG) << dbc->getCmdLineForFile(file.path);
            }

            dependenciesToTransform[currentFile->first] = currentFile->second;
            continue;
        }

        // Compare file modification time with the database.
        time_t dbModTime = dbc->getTimestampForFile(file.path);
        if (isFileInDb && file.modified != dbModTime) {
            LOG(logDEBUG) << "Modification time changed for file: " << file.path;
            LOG(logDEBUG) << "Current mod time: " << file.modified << " Database mod time: " << dbModTime;
            dependenciesToTransform[currentFile->first] = currentFile->second;
            continue;
        }

        // Compilation unit not changed checking comparing dependencies
        FileDatasVec dependenciesFromDb = dbc->getDependenciesList(file.path);
        FileDatasSet dependenciesFromFile = currentFile->second;
        std::sort(dependenciesFromDb.begin(), dependenciesFromDb.end());

        //BEWARE! std::equal can fail, if the given vectors are not the same dimension
        if (dependenciesFromDb.size() != dependenciesFromFile.size()) {
            dependenciesToTransform[currentFile->first] = currentFile->second;
            continue;
        }

        //BEWARE! std::equal can fail, if the given vectors are not the same dimension
        if (!std::equal(dependenciesFromDb.begin(), dependenciesFromDb.end(), dependenciesFromFile.begin())) {
            LOG(logDEBUG) << "File is in the database but dependencies are changed: " << file.path;
            dependenciesToTransform[currentFile->first] = currentFile->second;
            continue;
        }

        // Checking for dependencies modification time change
        if (!isFileDependenciesTimestampAreEqual(dependenciesFromDb, dependenciesFromFile)) {
            LOG(logDEBUG) << "Dependency timestamps are changed...";
            dependenciesToTransform[currentFile->first] = currentFile->second;
            //Add all dependencies to the list
        }
    }

    LOG(logINFO) << dependenciesToTransform.size() << " translation unit" << ((dependenciesToTransform.size() > 1) ? "s" : "") << " requires transformation.";
    for (auto it = dependenciesToTransform.begin(); it != dependenciesToTransform.end(); ++it) {
        LOG(logDEBUG) << "  Translation unit: " << it->first.path;
    }
}

bool BackportManager::isFileDependenciesTimestampAreEqual(std::vector<FileDatas>& vectorFromDb, std::set<FileDatas>& fromFile) {
    for (FileDatas fileFromDb : vectorFromDb) {
        auto pairFromFile = std::find_if(fromFile.begin(), fromFile.end(), [&fileFromDb](const FileDatas& fd) {
            return ((fd.path == fileFromDb.path) && (fd.modified == fileFromDb.modified));
        });

        //if pair is not found
        if (pairFromFile == fromFile.end()) {
            return false;
        }
    }
    return true;
}

std::vector<backport::helper::Path> BackportManager::getFilteredSources() {
    if (!useDatabase) {
        return getNonFilteredSources();
    }

    // Fills dependencies ToTransform vector
    makeListForTransformation();

    std::vector<backport::helper::Path> returnVector;

    for (auto currentFile = dependenciesToTransform.cbegin(), currentEnd = dependenciesToTransform.cend(); currentFile != currentEnd; ++currentFile) {

        returnVector.reserve(returnVector.size() + 1 + currentFile->second.size());

        returnVector.push_back(currentFile->first.path);

        for (auto const &currentDependency : currentFile->second) {
            returnVector.push_back(currentDependency.path);
        }
    }

    return returnVector;
}

std::vector<backport::helper::Path> BackportManager::getNonFilteredSources() {
    std::vector<backport::helper::Path> returnVector;

    for (auto currentFile = dependencies.cbegin(), currentEnd = dependencies.cend(); currentFile != currentEnd; ++currentFile) {

        returnVector.reserve(returnVector.size() + 1 + currentFile->second.size());

        returnVector.push_back(currentFile->first.path);

        for (auto currentDependency : currentFile->second) {
            returnVector.push_back(currentDependency.path);
        }
    }

    return returnVector;
}

void BackportManager::updateDatabase(){
    if (dependenciesToTransform.size() == 0) {
        return;
    }

    Timer timer;
    timer.reset();
    LOG(logINFO) << "Collecting files... Number of Dep: " << dependenciesToTransform.size();
    std::set<std::string> allFiles;
    for (auto currentFile = dependenciesToTransform.begin(), currentEnd = dependenciesToTransform.end(); currentFile != currentEnd; ++currentFile) {
        FileDatas file = currentFile->first;

        allFiles.insert(file.path);

        for (FileDatas dep : currentFile->second) {
            allFiles.insert(dep.path);
        }
    }

    LOG(logINFO) << "Collecting files... Done. Time: " << timer.duration();
    timer.reset();
    LOG(logINFO) << "Inserting files to database... Number of files: " << allFiles.size();
    dbc->beginTransaction();

    for (auto& file : allFiles) {
        if (!dbc->isFileInDb(file)) {
            dbc->addFile(file);
        }
    }

    dbc->endTransaction();
    LOG(logINFO) << "Inserting files to database... Done. " << " Time: " << timer.duration();
    timer.reset();
    LOG(logINFO) << "Inserting compilation units and relations to database...";

    for (auto currentFile = dependenciesToTransform.begin(), currentEnd = dependenciesToTransform.end(); currentFile != currentEnd; ++currentFile) {

        //Is file in the db
        FileDatas file = currentFile->first;
        bool isCompilationUnitInDb = dbc->isCompilationUnitInDb(file.path);

        if (!isCompilationUnitInDb) {

            dbc->beginTransaction();
            dbc->addCompilationUnit(file.path, file.modified, file.cmdLine);

            for (FileDatas dep : currentFile->second) {
                dbc->addRelation(file.path, dep.path, dep.modified);
            }
            dbc->endTransaction();
            continue;
        }

        if (isCompilationUnitInDb && (dbc->getCmdLineForFile(file.path) != file.cmdLine)) {
            dbc->updateCmdArguments(file.path, file.cmdLine);
        }

        //If timestamps are not equal
        if (dbc->getTimestampForFile(file.path) != file.modified) {
            auto dbDeps = dbc->getDependenciesList(file.path);
            dbc->updateTimestamp(file.path, file.modified);

            if (currentFile->second.size() != dbDeps.size() || !std::equal(currentFile->second.begin(), currentFile->second.end(), dbDeps.begin())) {
                dbc->beginTransaction();

                dbc->removeAllDependencies(file.path);
                for (FileDatas dep : currentFile->second) {
                    dbc->addRelation(file.path, dep.path, dep.modified);
                }

                dbc->endTransaction();
            }

            //dependencies are the same and timestamps are not
            if (!isFileDependenciesTimestampAreEqual(dbDeps, currentFile->second)) {
                dbc->beginTransaction();
                for (FileDatas dep : currentFile->second) {
                    dbc->updateTimestamp(dep.path, dep.modified);
                    dbc->updateRelation(file.path, dep.path, dep.modified);
                }
                dbc->endTransaction();
            }

        }

        dbc->beginTransaction();
        //Checking all the dependencies, if they are in the database. If so, refresh them, otherwise write them to the db
        for (FileDatas dep : currentFile->second) {
            //Refresh the current dependency, if it is in the db
            if (dbc->isFileInDb(dep.path)) {
                dbc->updateTimestamp(dep.path, dep.modified);
                dbc->updateRelation(file.path, dep.path, dep.modified);
            }
            //Write the current dependency to the db
            else {
                dbc->addRelation(file.path, dep.path, dep.modified);
            }
        }

        dbc->endTransaction();
    }
    LOG(logINFO) << "Inserting compilation units and relations to database... Done. Time: " << timer.duration();
}

void BackportManager::scanSources(CompilationDatabase &compilations, TransformOptions const& to) {
    ClangTool tool(compilations, backport::helper::containerPathToString(sources));
    // Dependencies couldn't be empty if we had a successful backport run earlier.
    IncludeActionFactory act(dependencies, to, compilations);
    if (tool.run(&act) != 0) {
        LOG(logERROR) << "Couldn't scan file dependencies!";
        exit(1);
    }
}
