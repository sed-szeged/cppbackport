#include "SourcePrepare.h"
#include "SimpleCompilationDatabase.h"
#include "PreprocessAction.h"

#include <algorithm>
#include <fstream>
#include <string>
#include <sstream>
#include <iostream>

#include "Util/Log.h"
#include "Util/TransformUtility.h"
#include "Util/PathUtility.h"

using namespace std;
using namespace clang::tooling;

namespace backport { using namespace helper;

    // Comparator for CompileCommand classes
    bool equalCommands(CompileCommand& lhs, CompileCommand& rhs) {
        if (lhs.Directory.compare(rhs.Directory) == 0) {
            if (lhs.CommandLine.size() == rhs.CommandLine.size()) {
                for (int i = 0; i < (int)lhs.CommandLine.size(); i++) {
                    if (lhs.CommandLine[i].compare(rhs.CommandLine[i]) != 0) {
                        return false;
                    }
                }
                if (lhs.MappedSources.size() == rhs.MappedSources.size()) {
                    auto lhsIt = lhs.MappedSources.begin();
                    auto rhsIt = rhs.MappedSources.begin();
                    auto lhsEnd = lhs.MappedSources.end();
                    while (lhsIt != lhsEnd) {
                        if (((*lhsIt).first.compare((*rhsIt).first) != 0) || ((*lhsIt).second.compare((*rhsIt).second) != 0)) {
                            return false;
                        }
                    }
                    return true;
                }
            }
        }
        return false;
    }

    // Filter and normalize the source files to get rid of possible duplicates
    vector<backport::helper::Path>& makeUniqueSources(vector<backport::helper::Path>& sources) {
        // A preliminary filtering to reduce the number of paths to be normalized
        std::sort(sources.begin(), sources.end());
        auto sourcesEnd = std::unique(sources.begin(), sources.end());
        sources.resize(std::distance(sources.begin(), sourcesEnd));

        for (auto& source : sources) {
            source.makeAbsolute();
        }

        // A final filter after all paths are normalized
        std::sort(sources.begin(), sources.end());
        sourcesEnd = std::unique(sources.begin(), sources.end());
        sources.resize(std::distance(sources.begin(), sourcesEnd));

        return sources;
    }

    // Make sure that path prefix we are attempting to replace in a string is actually a prefix
    void checkPathPrefix(const std::string& path, const std::string& prefix, const int& line) {
        if (prefix.at(prefix.size() - 1) != '/') {
            LOG(logERROR) << "Directory path does not end with '/'\n\tDirectory: " << prefix << "\n\tLine: " << line;
            exit(1);
        } else if (path.find(prefix) != 0) {
            LOG(logERROR) << "Directory path is not a prefix\n\tDirectory: " << prefix << "\n\tPath: " << path << "\n\tLine: " << line;
            exit(1);
        }
    }

    // Prepare the source files and the CompilationDatabase to use a different location
    void PrepareSources(unique_ptr<CompilationDatabase>& compilations, vector<backport::helper::Path>& sources, const string& constSourceRootDirectory, const string& constTargetDirectory, BackportManager &bm) {

        // Normalize all filepaths and filter out duplicates
        makeUniqueSources(sources);

        // The compilations should contain the base sourcefiles of the translation units
        vector<backport::helper::Path> sourceFiles = backport::helper::containerStringToPath(compilations->getAllFiles());
        makeUniqueSources(sourceFiles);

        // Get the compile commands from the compilation
        auto commands = compilations->getAllCompileCommands();

        // If the tool is used without a compile_commands.json file, getAllCompileCommands() will return empty
        if (commands.size() == 0) {
            for (auto& source : sources) {
                // In this case we can get the command for single files
                // We collect these while takeing care to avoid duplicates
                auto fileCommands = compilations->getCompileCommands(source.strRef());
                for (auto& cmd : fileCommands) {
                    bool contains = false;
                    for (auto& command : commands) {
                        if (equalCommands(cmd, command)) {
                            contains = true;
                        }
                    }
                    if (!contains) {
                        commands.push_back(cmd);
                    }
                }
            }
        }

        LOG(logDEBUG) << "Compile commands extracted";

        backport::helper::Path rootDirectory;
        // If the '-source-root' command line argument is not used, the default vale "AUTO DETECT" is passed
        if (constSourceRootDirectory.compare("AUTO DETECT") == 0) {
            // We determine the longest mutual prefix of all the files that we need to process
            // That will be considered the root directory
            auto* sourceContainer = &sources;
            if (sources.size() == 0) {
                if (sourceFiles.size() == 0) {
                    return;
                } else {
                    sourceContainer = &sourceFiles;
                }
            }

            rootDirectory = (*sourceContainer)[0];
            for (auto& sourceFile : (*sourceContainer)) {
                rootDirectory = rootDirectory.getMatchingPrefix(sourceFile);
            }

            rootDirectory.makeAbsolute();

            // If the tool is called on a single file, the longest matching prefix will be the full file path
            // In this case we need to get the directory containing the file
            if (!rootDirectory.isDirectory()) {
                rootDirectory = rootDirectory.getLastDir();
            }
        } else {
            // The root directory was passed as a command line parameter, we only need to make sure it's an absolute normalized path
            rootDirectory = Path(constSourceRootDirectory, true);
            rootDirectory.makeAbsolute();
        }
        
        LOG(logDEBUG) << "Root directory determined: " << rootDirectory;

        // The target directory needs to be normalized as well
        Path targetDirectory = Path(constTargetDirectory, true);

        Path destinationDirectory;
        
        // If the target is given as a relative path (like the default value of "temp", then we append it to the root directory and normalize it
        // NOTE : In this case we're not using makeAbsolute, because that would append the relative path to where the application is ran from, whitch is not ideal
        if (!targetDirectory.isAbsolute()) {
            destinationDirectory = targetDirectory.makeAbsoluteFrom(rootDirectory);
        } else {
            destinationDirectory = targetDirectory;
        }
        
        LOG(logDEBUG) << "Destination directory determined: " << destinationDirectory;
        
        // In large projects the linkgenerator should be used beforhand to create the proper file structure
        // But the backport tool can also create the necessary folders
        if (!llvm::sys::fs::exists(destinationDirectory.twine())) {
            LOG(logDEBUG) << "Created directory " << destinationDirectory.str();
            llvm::sys::fs::create_directory(destinationDirectory.twine());
        }
        
        // Begin processing the compile commands
        // We need to alter all paths that use the original root directory, to insted use the new target directory
        for (auto& command : commands) {

            LOG(logDEBUG) << "Command directory: " << command.Directory;

            Path cmdDir = command.Directory;
            // If the directory is relative, set it to the destination directory
            // NOTE : This should not occur when using compile_commands.json file
            if (!cmdDir.isAbsolute()) {
                cmdDir.makeAbsoluteFrom(destinationDirectory);
            } else {

                // This is only for debugging, should only occur with an incorrect json file
                if (cmdDir.str().size() < rootDirectory.str().size()) {
                    for (auto& cmd : command.CommandLine) {
                        LOG(logDEBUG) << "  Command: " << cmd;
                    }
                }

                // The rootDirectory prefix is replaced for the destinationDirectory
                checkPathPrefix(cmdDir, rootDirectory, __LINE__);
                cmdDir.replacePathPrefix(rootDirectory, destinationDirectory);
            }
            // removeSlashFromPath(cmdDir);
            command.Directory = cmdDir;

            LOG(logDEBUG) << "New cmd directory: " << command.Directory;

            auto& commandline = command.CommandLine;

            for (auto& cmd : commandline) {

                LOG(logDEBUG) << "  Command: " << cmd;

                // We are only interested in command line parameters describing file paths
                auto cmd_copy = cmd;
                if ((cmd_copy.at(0) != '-') && (cmd_copy.compare("clang") != 0)) {
                    Path cmd_path(cmd_copy);
                    // If the command turned into a filepath is a file that exists the command was probably a filepath originally
                    // If the path has the rootDirectory as a prefix then we'll change it for the destinationDirectory
                    // NOTE : This could go wrong if a command that is actually not a filepath when made absolute turns out to be an existing file. Highly unlikely
                    if ((llvm::sys::fs::exists(cmd_path.twine())) && cmd_path.hasPrefix(rootDirectory)) {
                        checkPathPrefix(cmd_copy, rootDirectory, __LINE__);
                        cmd = cmd_path.replacePathPrefix(rootDirectory, destinationDirectory);
                    }
                } else if (cmd_copy.substr(0, 2).compare("-I") == 0) {
                    // We change all include paths that have the rootDirectory as a prefix
                    // NOTE : all include paths that relate to the transformed files should be in the format '-I<ABSOLUTE FILEPATH>'
                    //  This is something that can cause problems if the json file is not made following this rule
                    Path includePath = cmd_copy.substr(2);
                    if (includePath.hasPrefix(rootDirectory)) {
                        checkPathPrefix(includePath, rootDirectory, __LINE__);
                        cmd = std::string("-I") + includePath.replacePathPrefix(rootDirectory, destinationDirectory).str();
                    }
                }
            }
        }

        LOG(logDEBUG) << "Commands processed.";
        
        // After the compile commands are processed, we alter the filepaths
        // We don't check here if the rootDirectory is a prefix, because all files that are to be transformed should be within the rootDirectory
        // This can be invalidated if the 'source-root' argument given is incorrect
        for (auto& sourceFile : sourceFiles) {
            checkPathPrefix(sourceFile, rootDirectory, __LINE__);
            sourceFile = sourceFile.replacePathPrefix(rootDirectory, destinationDirectory);
        }

        LOG(logDEBUG) << "Source file paths modified.";

        // Start copying the files
        int copies = 0;
        vector<Path> filteredSources;
        for (auto& source : sources) {
            ifstream input;

            // Really paranoid after a lot of trouble with filepath formats
            source.makeAbsolute();

            input.open(source);
            checkPathPrefix(source, rootDirectory, __LINE__);
            Path destination = source;
            destination.replacePathPrefix(rootDirectory, destinationDirectory);

            // Here we make sure that the destination directory exists
            // If it does not, we create it, with the directories leading up to it if necessary
            Path destinationSubDirectory = destination.getLastDir();
            std::list<Path> directories;
            while (!llvm::sys::fs::exists(llvm::Twine(destinationSubDirectory)))
            {
                directories.push_front(destinationSubDirectory);
                destinationSubDirectory = destinationSubDirectory.getOneStepUp();
            }

            for (auto dir : directories) {
                LOG(logDEBUG) << "Created directory " << dir.str();
                llvm::sys::fs::create_directory(llvm::Twine(dir));
            }

            // Do not overwrite original source files
            if (destination.str().compare(source) == 0) {
                LOG(logERROR) << "Destination directory is the same as the source directory";
                exit(1);
            }

            // Check for hard links, we don't want to overwrite those
            // TODO : What about soft links?
            llvm::Twine sourceTwine(source);
            llvm::sys::fs::file_status statsrc;
            llvm::sys::fs::status(sourceTwine, statsrc);
            llvm::Twine destinationTwine(destination);
            llvm::sys::fs::file_status statdst;
            llvm::sys::fs::status(destinationTwine, statdst);
            if (llvm::sys::fs::exists(statsrc) && llvm::sys::fs::exists(statdst) && statsrc.getUniqueID() == statdst.getUniqueID()) {
                LOG(logERROR) << "Destination file and source file has the same unique ID. Possible hard link.\n" << "\tSource path: " << source << "\n\tDestination path: " << destination;
                exit(1);
            }

            // We use binary to avoid automatic translation of endline to include CR on windows systems
            ofstream output;
            output.open(destination, std::ios_base::binary | std::ios_base::out);
            copies++;

            if (bm.useDB())
                bm.getDb()->resetLineForFile(destination);

            bool included = false;
            bool stop = false;
            string line;
            int currentLineNumber = 0;
            int insertionLineNumber = 0;
            while (!stop) {
                stop = getline(input, line).eof();
                currentLineNumber += 1;

                // Hack for fixing a bug that occurs when force late parsing old Visual Studio header files
                if (
                      (!included) && (source.str().substr(source.str().size() - 2).compare(std::string(".c")) != 0) &&
                      (
                        (line.find(std::string("#include <iostream>")) != std::string::npos) ||
                        (line.find(std::string("#include <string>")) != std::string::npos) ||
                        (line.find(std::string("#include <utility>")) != std::string::npos) ||
                        (line.find(std::string("#include <ios>")) != std::string::npos)
                      )
                   ) {
                    output << "#include <locale>\n";
                    included = true;
                    insertionLineNumber = currentLineNumber;
                    //output << "namespace std { template<class _Elem> class collate; }\n";
                }

                output << line;
                if (!stop) {
                    output << endl;
                }
            }

            //shift the lines because of the <locale> include;
            if (bm.useDB() && included) {
                auto dbc = bm.getDb();

                if (dbc->isFileInDb(destination) == false)
                    dbc->addFile(destination);

                for (int i = currentLineNumber + 1; i >= insertionLineNumber; --i) {
                    dbc->updateLineOffsetMapping(destination, i + 1, dbc->getOriginalLineNumber(destination, i));
                }
            }

            input.close();
            output.close();

            checkPathPrefix(source, rootDirectory, __LINE__);
            source.replacePathPrefix(rootDirectory, destinationDirectory);
            // Filter out the files to compile
            llvm::Regex regex("\\.(cpp|c|cxx|cc)$");
            if (regex.match(source.str())) {
                filteredSources.push_back(source);
            }
        }
        sources = std::move(filteredSources);

        LOG(logINFO) << copies << " file" << ((copies > 1) ? "s" : "") << " copied";

        // Create our version of the CompilationDatabase
        std::map< std::string, std::vector<CompileCommand> > fileCommandMap;
        std::vector<Path> relevantSourceFiles;

        sourceFiles = sources;
        for (auto it = sourceFiles.crbegin(); it != sourceFiles.crend(); ++it) {
            std::vector<CompileCommand> fileCommands;
            Path filePath = *it;
            std::string const filePath_as_string = filePath;

            // For every file we add the compile commands that contain that filepath as a command line parameter
            for (auto const &compileCommand : commands) {
                bool relevant = false;
                for (auto const &cmd : compileCommand.CommandLine) {
                    if (cmd.compare(filePath_as_string) == 0) {
                        relevant = true;
                    }
                }
                if (relevant) {
                    fileCommands.push_back(compileCommand);
                }
            }

            // If the file has any compile commands, we add it for further processing
            if (fileCommands.size() > 0) {
                fileCommandMap[filePath] = fileCommands;
                relevantSourceFiles.push_back(filePath);
            }
        }

        // At this point only the base translation unit sourcefiles should be contained in sources
        sources = relevantSourceFiles;

        // We replace the original CompilationDatabase with our version that has all relevant filepaths modified
        compilations.release();
        auto pathStr = containerPathToString(relevantSourceFiles);
        compilations = unique_ptr<SimpleCompilationDatabase>(new SimpleCompilationDatabase(commands, pathStr, fileCommandMap));

#ifdef PREPROCESS_COMP_UNITS
        {
            auto preproc_only_commands = commands;

            for (auto &current : preproc_only_commands) {
                current.CommandLine.push_back("-P");
            }

            auto compilations_for_preproc = unique_ptr<SimpleCompilationDatabase>(new SimpleCompilationDatabase(preproc_only_commands, helper::containerPathToString(relevantSourceFiles), fileCommandMap));

            ClangTool tool(*compilations_for_preproc.get(), helper::containerPathToString(sources));
            PreprocessActionFactory act;

            if (tool.run(&act) != 0) {
                LOG(logERROR) << "Couldn't preprocess the compilation units! ";
                abort();
            }

            LOG(logDEBUG) << "  Files has been preprocessed.";

        }

#endif
        return;
    }
}
