#ifndef _PREPROCESSACTION_HEADER_
#define _PREPROCESSACTION_HEADER_

#include <clang/Frontend/FrontendActions.h>
#include <clang/Tooling/Tooling.h>
#include <clang/Frontend/CompilerInstance.h>

using namespace clang;
using namespace clang::tooling;
using namespace backport;

namespace backport {

    class BackportPrintPreprocessedAction : public PrintPreprocessedAction {
    public:
        virtual bool BeginInvocation(CompilerInstance &CI) override {
            auto &DepOutputOpts = CI.getDependencyOutputOpts();
            auto &PreprocOutputOpts = CI.getPreprocessorOutputOpts();
            auto &FrontendOpts = CI.getFrontendOpts();

            DepOutputOpts.IncludeSystemHeaders = 0;
            DepOutputOpts.IncludeModuleFiles = 0;
            //DepOutputOpts.OutputFile = this->getCurrentFile();

            PreprocOutputOpts.ShowCPP = 1;
            PreprocOutputOpts.RewriteIncludes = 1;
            PreprocOutputOpts.ShowMacros = 0;

            FrontendOpts.OutputFile = this->getCurrentFile();
            FrontendOpts.FixWhatYouCan = 1;

            return true;
        }
    };

    class PreprocessActionFactory : public FrontendActionFactory
    {
    public:
        PreprocessActionFactory() {}

        virtual FrontendAction *create() {
            return new BackportPrintPreprocessedAction();
        }
    };
}

#endif
