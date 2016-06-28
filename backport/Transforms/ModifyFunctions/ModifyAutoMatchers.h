#ifndef BACKPORT_MODIFY_AUTO_MATCHERS_H
#define BACKPORT_MODIFY_AUTO_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *ModAutoFuncDeclId = "mod_auto_function_decl";
const char *ModAutoFuncSpecDeclId = "mod_auto_function_specialization_decl";
const char *ModAutoFuncCallId = "mod_auto_function_call";

namespace clang { namespace ast_matchers {

    // NOTE : We can't actually check for auto functions or suffix return type syntax
    //  so instead we have to check for return types given with decltype, since that's the most common use-case

    // Matcher for auto return type function where the return type is dependent
    DeclarationMatcher makeAutoFuncDeclMatcher() {
        return
            functionTemplateDecl(
                hasTrailingReturnType(),
                hasDependentReturnType()
            ).bind(ModAutoFuncDeclId);
    }

    // Matcher for specialization declarations of auto return type functions
    DeclarationMatcher makeAutoFuncSpecDeclMatcher() {
        return
            functionDecl(
                hasTrailingReturnType(),
                hasFunctionTemplate( functionTemplateDecl(hasDependentReturnType()) ),
                isDeclarationOnly()
            ).bind(ModAutoFuncSpecDeclId);
    }

    // Matcher for calls to auto return type functions
    StatementMatcher makeAutoFuncCallMatcher() {
        return
            callExpr(
                callee(
                    functionDecl(
                        isTemplateSpecialization(),
                        hasTrailingReturnType(),
                        hasFunctionTemplate( functionTemplateDecl(hasDependentReturnType()) )
                    )
                )
            ).bind(ModAutoFuncCallId);
    }

    // Feature Finder Matcher
    DeclarationMatcher findModifyAutoDeclMatcher() {
        return
            decl(
                anyOf(
                    makeAutoFuncDeclMatcher(),
                    makeAutoFuncSpecDeclMatcher()
                )
            ).bind(FoundDeclID);
    }

    StatementMatcher findModifyAutoStmtMatcher() {
        return
            stmt(
                makeAutoFuncCallMatcher()
            ).bind(FoundStmtID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_MODIFY_AUTO_MATCHERS_H
