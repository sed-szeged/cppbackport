#include "TransformBase/TransformHandler.h"

#include "DeduceAuto.inc"
#include "TypeAlias.inc"
#include "ReplaceMemberInit.inc"
#include "ReplaceForRange.inc"

#include "TypeAliasMatchers.h"
#include "DeduceAutoMatchers.h"
#include "ReplaceMemberInitMatchers.h"
#include "ReplaceForRangeMatchers.h"

const char MultipleTransformsID[] = "MultipleTransforms";

struct MultipleTransformsFactory : TransformFactory {
    // auto clang 2.9, gcc 4.4, icc 12, msvc 10
    // template alias clang 3.0, gcc 4.7, icc 12, msvc 13
    MultipleTransformsFactory() {
        // latest compiler version which supports all of the features
        Since.Clang = Version(3, 0);
        Since.Gcc = Version(4, 7);
        Since.Icc = Version(14);
        Since.Msvc = Version(13);

        aliasTemplateVersion.Clang = Version(3, 0);
        aliasTemplateVersion.Gcc = Version(4, 7);
        aliasTemplateVersion.Icc = Version(12);
        aliasTemplateVersion.Msvc = Version(13);

        autoVersion.Clang = Version(2, 9);
        autoVersion.Gcc = Version(4, 4);
        autoVersion.Icc = Version(12);
        autoVersion.Msvc = Version(10);

        rangeBasedForVersion.Clang = Version(3, 0);
        rangeBasedForVersion.Gcc = Version(4, 6);
        rangeBasedForVersion.Icc = Version(13);
        rangeBasedForVersion.Msvc = Version(12);

        memberInitVersion.Clang = Version(3, 0);
        memberInitVersion.Gcc = Version(4, 7);
        memberInitVersion.Icc = Version(14);
        memberInitVersion.Msvc = Version(13);
    }

    Transform *createTransform(const TransformOptions &Opts) override {
        TransformHandler *handler = new TransformHandler(MultipleTransformsID, Opts, TransformPriorities::MULTIPLE_TRANSFORMS);
        auto &acc = handler->getAcceptedChanges();

        if (!supportsCompilers(Opts.TargetVersions, autoVersion, false)) {
            handler->initReplacers(
                new SingleAutoReplacer(acc, *handler), makeSingleAutoDeclMatcher,
                new MultipleAutoReplacer(acc, *handler), makeMultipleAutoDeclMatcher,
                new AutoWithoutDeclStmtReplacer(acc, *handler), makeAutoDeclarationMatcherWithoutDeclStmt,
                new MethodWithDeclTypeReplacer(acc, *handler), makeDeclTypeMethodMatcher
                );
        }

        if (!supportsCompilers(Opts.TargetVersions, aliasTemplateVersion, false)) {
            handler->initReplacers(
                new TypedefTypeAliasReplacer(acc, *handler), makeTypeAliasInTypedefDeclMatcher,
                new TypeAliasDeclReplacer(acc, *handler), makeTypeAliasDeclMatcher,
                new TypeAliasTemplateVarTypeReplacer(acc, *handler), makeVariableWithTypeTemplateAliasDeclMatcher,
                new TypeAliasTemplateDeclConverter(acc, *handler), makeTypeAliasTemplateDeclMatcher,
                new TypeAliasTemplateReturnTypeReplacer(acc, *handler), makeFunctionReturnTypeTemplateAliasDeclMatcher,
                new TypeAliasTemplateTemplateArgumentReplacer(acc, *handler), makeTemplateArgumentTemplateAliasDeclMatcher,
                new TypeAliasTemplateExprReplacer(acc, *handler), makeTemplateTypeAliasContainedExprMatcher,
                new TypeAliasTemplateClassTemplateSpecializationReplacer(acc, *handler), makeClassTemplateSpecializationDeclMatcher,
                new TypeAliasTemplateFunctionTemplateSpecializationReplacer(acc, *handler), makeFunctionTemplateSpecializationDeclMatcher,
                new TypeAliasTemplateClassMethodNameReplacer(acc, *handler), makeClassMethodNameTemplateTypeAliasDeclMatcher
                );
        }

        if (!supportsCompilers(Opts.TargetVersions, rangeBasedForVersion, false)) {
            handler->initReplacers(
                new ForRangeReplacer(
                    handler->getAcceptedChanges(), *handler), makeForRangeStmtMatcher
                );
        }

        if (!supportsCompilers(Opts.TargetVersions, memberInitVersion, false)) {
            handler->initReplacers(new CtorMemberInitReplacer(handler->getAcceptedChanges(), *handler), makeConstructorInClassMemberInitMatcher,
                new RecordMemberInitWithoutCtorReplacer(handler->getAcceptedChanges(), *handler), makeClassInClassMemberInitWithoutCtorMatcher,
                new RecordDeclMemberInitRemover(handler->getAcceptedChanges(), *handler), makeRecordDeclMatcherWithInClassInit
                );
        }
        return handler;
    }

    CompilerVersions aliasTemplateVersion;
    CompilerVersions autoVersion;
    CompilerVersions memberInitVersion;
    CompilerVersions rangeBasedForVersion;
};

// Register the factory using this statically initialized variable.
static TransformFactoryRegistry::Add<MultipleTransformsFactory>
X("multiple-transforms", "Contains multiple transformations:\n- deduction of 'auto' type specifier\n- (template) type aliases\n- range-based for\n- in-class member initializations");
