#ifndef _INCLUDEACTION_HEADER_
#define _INCLUDEACTION_HEADER_

#include <clang/Frontend/FrontendAction.h>
#include <clang/Frontend/CompilerInstance.h>
#include <clang/Lex/Preprocessor.h>
#include "TransformBase/Tooling.h"
#include "Util/FileDatas.h"
#include "Util/PathUtility.h"

using namespace clang;
using namespace clang::tooling;
using namespace backport;

struct TransformOptions;

namespace backport {

    /// \brief PPCallbacks that fills-in the include information in the given
    /// \c set.
    class IncludeDirectivesPPCallback : public clang::PPCallbacks
    {
    public:
        IncludeDirectivesPPCallback(std::set<const FileEntry*> &incs)
            : includes(incs) {}
        virtual ~IncludeDirectivesPPCallback() {}

    private:
        void InclusionDirective(SourceLocation HashLoc, const Token &IncludeTok, StringRef FileName, bool IsAngled, CharSourceRange FilenameRange, const FileEntry *File, StringRef SearchPath, StringRef RelativePath, const Module *Imported) override {
            includes.insert(File);
        }

        std::set<const FileEntry*> &includes;
    };


    class IncludeAction : public PreprocessorFrontendAction
    {
    public:
        IncludeAction(DependencyMap& dependencies, TransformOptions const& to, CompilationDatabase& compilations) : dependencies(dependencies), to(to), compilations(compilations) {}

        bool BeginSourceFileAction(CompilerInstance &CI, StringRef Filename);
        bool isCompilationUnitOrDependenciesChanged();
        void ExecuteAction();

    private:
        FileDatas currentFile;
        DependencyMap& dependencies;
        TransformOptions const& to;
        CompilationDatabase& compilations;
    };

    class IncludeActionFactory : public FrontendActionFactory
    {
    public:
        IncludeActionFactory(DependencyMap& dependencies, TransformOptions const& to, CompilationDatabase &compilations) : dependencies(dependencies), to(to), compilations(compilations) {}

        virtual FrontendAction *create() {
            return new IncludeAction(dependencies, to, compilations);
        }

    private:
        DependencyMap& dependencies;
        TransformOptions const& to;
        CompilationDatabase& compilations;
    };
}

#endif
