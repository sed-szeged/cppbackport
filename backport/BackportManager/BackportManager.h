#ifndef BACKPORT_MANAGER_H
#define BACKPORT_MANAGER_H

#include "clang/Basic/FileManager.h"
#include <clang/Tooling/CompilationDatabase.h>
#include "Database/DatabaseConnection.h"
#include "Util/FileDatas.h"
#include "Util/PathUtility.h"

struct TransformOptions;

namespace backport {
    class BackportManager {
    public:
        BackportManager(std::vector<backport::helper::Path> sources, const std::string& dbFile, bool usDb, bool resetDb, bool child = false);

        ~BackportManager() ;

        /**
         * Read the source files and  their dependencies
         */
        void scanSources(clang::tooling::CompilationDatabase& compilations, TransformOptions const& to);

        /**
         * Get the filtered file list.
         */
        std::vector<backport::helper::Path> getFilteredSources();

        /**
         * After finishing transformations, this method updates the database
         */
        void updateDatabase();

        /**
         * Get the dbc handle. (by reference).
         */
        std::shared_ptr<DatabaseInterface> getDb() {
            assert(dbc && "A database must exists in order to get it. To use this function you must"
                "have at least one instance of BackportManager (which initializes the database connection).");
            return dbc;
        }

        bool useDB() { return useDatabase; }

        /**
        * Store input sources and their dependencies
        */
        std::map<FileDatas, std::set<FileDatas>> dependencies;

    private:

        /**
        * Get the list of files for transformation.
        */
        std::vector<backport::helper::Path> getNonFilteredSources();

        /**
         * Load dependencies from database if available.
         */
        void loadDependencies();

        /**
         * Makes the list for transformation, by uploading the member dependenciesToTransform
         */
        void makeListForTransformation();

        /**
          * Checks if the given parameters are equal (by content)
          */
        bool isFileDependenciesTimestampAreEqual(std::vector<FileDatas>&, std::set<FileDatas>&);

        /**
         * Database connection
         */
        std::shared_ptr<DatabaseInterface> dbc;

        /**
         * Store input sources
         */
        std::vector<backport::helper::Path> sources;

        /**
         * Store input sources and dependencies which should be transformed
         */
        std::map<FileDatas, std::set<FileDatas>> dependenciesToTransform;

        /**
         * True if database is used.
         */
        bool useDatabase;
    };
}

#endif
