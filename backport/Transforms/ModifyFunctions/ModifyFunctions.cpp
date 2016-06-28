#include "TransformBase/TransformHandler.h"

#include "ModifyAuto.inc"
#include "ModifyVariadic.inc"

#include "ModifyAutoMatchers.h"
#include "ModifyVariadicMatchers.h"

const char ModifyFunctionsID[] = "ModifyFunctions";

struct ModifyFunctionsFactory : TransformFactory {
    ModifyFunctionsFactory() {
        Since.Clang = Version(2, 9);
        Since.Gcc = Version(4, 4);
        Since.Icc = Version(12);
        Since.Msvc = Version(13);

        autoFunctionVersion.Clang = Version(2, 9);
        autoFunctionVersion.Gcc = Version(4, 4);
        autoFunctionVersion.Icc = Version(12);
        // NOTE : Trailing return type is supported since VS 2010
        // But decltype is only fully supported since VS 2013
        autoFunctionVersion.Msvc = Version(13);

        variadicFunctionVersion.Clang = Version(2, 9);
        variadicFunctionVersion.Gcc = Version(4, 4);
        variadicFunctionVersion.Icc = Version(12);
        variadicFunctionVersion.Msvc = Version(13);
    }

    Transform *createTransform(const TransformOptions &Opts) override {
        TransformHandler *handler = new TransformHandler(ModifyFunctionsID, Opts, TransformPriorities::MODIFY_FUNCTIONS);
        auto &acc = handler->getAcceptedChanges();

        if (!supportsCompilers(Opts.TargetVersions, autoFunctionVersion, false)) {
            handler->initReplacers(
                new AutoFuncDeclReplacer(acc, *handler), makeAutoFuncDeclMatcher,
                new AutoFuncSpecDeclReplacer(acc, *handler), makeAutoFuncSpecDeclMatcher,
                new AutoFuncCallReplacer(acc, *handler), makeAutoFuncCallMatcher
                );
        }

        if (!supportsCompilers(Opts.TargetVersions, variadicFunctionVersion, false)) {
            handler->initReplacers(
                new NonVariadicParamNumFinder(handler->getAcceptedChanges(), *handler), makeVariadicFinalFunctionVarCountMatcher,
                new VariadicFuncCallReplacer(handler->getAcceptedChanges(), *handler), makeVariadicFuncCallMatcher,
                new VariadicFuncFinalCallReplacer(handler->getAcceptedChanges(), *handler), makeVariadicFinalFunctionMatcher,
                new VariadicTemplateFunctionModifier(handler->getAcceptedChanges(), *handler), makeVariadicTemplateFunctionMatcher
                );
        }

        return handler;
    }

    CompilerVersions autoFunctionVersion;
    CompilerVersions variadicFunctionVersion;
};

// Register the factory using this statically initialized variable.
static TransformFactoryRegistry::Add<ModifyFunctionsFactory>
X("modify-functions", "Modifies auto return type and variadic functions");
