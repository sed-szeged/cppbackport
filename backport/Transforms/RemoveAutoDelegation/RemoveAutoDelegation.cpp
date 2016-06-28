#include "TransformBase/TransformHandler.h"

#include "RemoveAuto.inc"
#include "DelegatingConstructor.inc"

#include "RemoveAutoMatchers.h"
#include "DelegatingConstructorMatchers.h"

const char RemoveAutoDelegationID[] = "RemoveAutoDelegation";

struct RemoveAutoDelegationFactory : TransformFactory {
    RemoveAutoDelegationFactory() {
        Since.Clang = Version(3, 0);
        Since.Gcc = Version(4, 7);
        Since.Icc = Version(14);
        Since.Msvc = Version(13);

        delegatingVersion.Clang = Version(3, 0);
        delegatingVersion.Gcc = Version(4, 7);
        delegatingVersion.Icc = Version(14);
        delegatingVersion.Msvc = Version(13);

        removeAutoVersion.Clang = Version(2, 9);
        removeAutoVersion.Gcc = Version(4, 4);
        removeAutoVersion.Icc = Version(12);
        removeAutoVersion.Msvc = Version(10);
    }

    Transform *createTransform(const TransformOptions &Opts) override {
        TransformHandler *handler = new TransformHandler(RemoveAutoDelegationID, Opts, TransformPriorities::REMOVE_AUTO_DELEGATION);
        auto &acc = handler->getAcceptedChanges();

        if (!supportsCompilers(Opts.TargetVersions, delegatingVersion, false)) {
            handler->initReplacers(
                new DelegatingConstructorReplacer(acc, *handler), makeDelegatingConstructorMatcher
                );
        }

        if (!supportsCompilers(Opts.TargetVersions, removeAutoVersion, false)) {
            handler->initReplacers(
                new AutoFuncDeclRemover(acc, *handler), makeAutoFuncDeclRemMatcher,
                new AutoTemplateFuncDeclRemover(acc, *handler), makeAutoTemplateFuncDeclRemMatcher,
                new AutoTemplateClassDeclRemover(acc, *handler), makeAutoTemplateClassDeclRemMatcher
                );
        }
        return handler;
    }

    CompilerVersions delegatingVersion;
    CompilerVersions removeAutoVersion;
};

// Register the factory using this statically initialized variable.
static TransformFactoryRegistry::Add<RemoveAutoDelegationFactory>
X("remove-auto-delegation", "Contains multiple transformations:\n- Removes unused auto containing functions and classes\n- Transforms constructor delegations");
