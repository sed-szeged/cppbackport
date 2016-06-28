#ifndef SOURCE_PREPARE_H
#define SOURCE_PREPARE_H

#include <string>
#include <vector>
#include <clang/Tooling/CompilationDatabase.h>
#include "BackportManager.h"

namespace backport {

    // Prepare the source files and the CompilationDatabase to use a different location
    void PrepareSources(
        std::unique_ptr<clang::tooling::CompilationDatabase>& compilations,
        std::vector<backport::helper::Path>& sources,
        const std::string& constTargetRootDirectory,
        const std::string& constTargetDirectory,
        BackportManager &bm);

}

#endif //SOURCE_PREPARE_H
