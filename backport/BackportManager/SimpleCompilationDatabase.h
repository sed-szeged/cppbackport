#ifndef SIMPLE_COMPILATION_DATABASE_H
#define SIMPLE_COMPILATION_DATABASE_H

#include "clang/Basic/LLVM.h"
#include "llvm/ADT/StringRef.h"
#include <memory>
#include <string>
#include <vector>
#include <map>
#include "Util/TransformUtility.h"
#include "Util/StringUtility.h"
#include "Util/PathUtility.h"

using namespace backport::helper;

namespace clang { namespace tooling {

    // A class that mimics the other CompilationDatabase classes but has very little functionality
    // On its own it can't generate a compilation database, it can only return the values that are stored within it at construction
    class SimpleCompilationDatabase : public CompilationDatabase {
    public:
        SimpleCompilationDatabase(std::vector<CompileCommand>& compileCommands, std::vector<std::string>& files, std::map< std::string, std::vector<CompileCommand> > fileCommandMap) : compileCommands(compileCommands), files(files), fileCommandMap(fileCommandMap) {}

        SimpleCompilationDatabase(std::map< std::string, std::vector<CompileCommand> > fileCommandMap) : fileCommandMap(fileCommandMap) {
            for (const auto& pair : fileCommandMap) {
                files.push_back(pair.first);
                for (const auto& command : pair.second) {
                    compileCommands.push_back(command);
                }
            }
        }

        SimpleCompilationDatabase(const CompilationDatabase& database, const std::vector< std::string > sourcefiles) {
            for (const auto& sourcefile : sourcefiles) {
                std::string source = backport::helper::Path(sourcefile).str();
                fileCommandMap[source] = database.getCompileCommands(sourcefile);
            }
            for (const auto& pair : fileCommandMap) {
                files.push_back(pair.first);
                for (const auto& command : pair.second) {
                    compileCommands.push_back(command);
                }
            }
        }

        std::vector<CompileCommand> getCompileCommands(StringRef FilePath) const override {
            std::vector<CompileCommand> fileCommands;
            std::string filePath = backport::helper::Path(FilePath).str();

            if (fileCommandMap.count(filePath) != 0) {
                fileCommands = fileCommandMap.find(filePath)->second;
            }

            return fileCommands;
        }

        std::vector<std::string> getAllFiles() const override { return files; }

        std::vector<CompileCommand> getAllCompileCommands() const override { return compileCommands; }

    private:
        std::vector<CompileCommand> compileCommands;
        std::vector<std::string> files;
        std::map< std::string, std::vector<CompileCommand> > fileCommandMap;
    };

} /* end namespace tooling */ } /* end namespace clang */

#endif // SIMPLE_COMPILATION_DATABASE_H
