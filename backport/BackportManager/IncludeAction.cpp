#include "IncludeAction.h"
#include "TransformBase/Tooling.h"
#include "TransformBase/Transform.h"
#include "Util/Log.h"

namespace backport {

bool IncludeAction::BeginSourceFileAction(CompilerInstance &CI, StringRef Filename) {
    auto currentFileEntry = CI.getFileManager().getFile(Filename);
    currentFile.path = currentFileEntry->getName();
    currentFile.modified = currentFileEntry->getModificationTime();

    std::stringstream argsStream;
    for (auto compileCommands : compilations.getCompileCommands(Filename)) {
        for (auto cmdLineArg : compileCommands.CommandLine)
            argsStream << " " << cmdLineArg;
    }
    currentFile.cmdLine = argsStream.str();
    // checking for previous existance of this compilation unit
    if (dependencies.count(currentFile) && !isCompilationUnitOrDependenciesChanged()) {
        return false;
    }

    return PreprocessorFrontendAction::BeginSourceFileAction(CI, Filename);
}

bool IncludeAction::isCompilationUnitOrDependenciesChanged() {
    auto cu = dependencies.find(currentFile)->first;
    auto deps = dependencies[currentFile];

    // timestamps or arguments
    if (cu.modified != currentFile.modified || cu.cmdLine != currentFile.cmdLine) {
        return true;
    }
    // dependencies
    for (FileDatas dep : deps) {
        auto currDep = getCompilerInstance().getFileManager().getFile(dep.path.strRef());
        // Is continue the proper way? Maybe it is ok because removing a source file may cause other files to change.
        if (!currDep) {
            LOG(logWARNING) << "    Not existing dependency file. Path: " << dep.path;
            continue;
        }

        if (dep.modified != currDep->getModificationTime()) {
            return true;
        }
    }
    return false;
}

void IncludeAction::ExecuteAction() {
    std::set<const FileEntry*> includes;
    Preprocessor &pp(getCompilerInstance().getPreprocessor());
    pp.addPPCallbacks(llvm::make_unique<IncludeDirectivesPPCallback>(includes));
    pp.EnterMainSourceFile();

    Token token;
    do {
        pp.Lex(token);
    } while (token.isNot(tok::eof));


    if (dependencies.count(currentFile)) {
        dependencies.erase(currentFile);
    }
    //We want to keep files which does not have any include
    dependencies[currentFile] = std::set<FileDatas>();

    std::for_each(includes.begin(), includes.end(), [this](const FileEntry* in) {
        if (to.ModifiableFiles.isFileIncluded(in->getName())) {
            helper::Path path = in->getName();
            dependencies[currentFile].insert({ path.str(), in->getModificationTime() });
        }
    });
}

} // namespace backport
