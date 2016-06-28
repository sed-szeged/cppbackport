#ifndef BACKPORT_REMOVE_AUTO_MATCHERS_H
#define BACKPORT_REMOVE_AUTO_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *RemAutoFunctionId = "rem_auto_function";
const char *RemAutoTemplFunctionId = "rem_auto_templ_function";
const char *RemAutoTemplClassId = "rem_auto_templ_class";

namespace clang { namespace ast_matchers {

    // Matcher for removing functions and classes that contain auto expressions
    // Since this transformation is after deduce auto, the fact that auto expressions still remain mean
    //  that the function/class was never used in any of the translation units

    // Matcher for auto containing functions
    DeclarationMatcher makeAutoFuncDeclRemMatcher() {
        return
            functionDecl(
                unless( isImplicit() ),
                unless( functionDecl(isExplicitTemplateSpecialization()) ),
                unless( functionDecl(isInstantiated()) ),
                unless( functionDecl(hasParent(functionTemplateDecl())) ),
                unless( hasAncestor(classTemplateDecl(hasDescendant(autoType()))) ),
                anyOf(
                    functionDecl(
                        isExpansionInMainFile(),
                        hasTrailingReturnType()
                    ),
                    functionDecl(
                        isExpansionInMainFile(),
                        hasDescendant( autoType() )
                    )
                )
            ).bind(RemAutoFunctionId);
    }

    // Matcher for auto containing template functions
    DeclarationMatcher makeAutoTemplateFuncDeclRemMatcher() {
        return
            functionTemplateDecl(
                isExpansionInMainFile(),
                unless( isImplicit() ),
                unless( hasAncestor(classTemplateDecl(hasDescendant(autoType()))) ),
                anyOf(
                    functionTemplateDecl(
                        hasTrailingReturnType()
                    ),
                    functionTemplateDecl(
                        hasDescendant( autoType() )
                    )
                )
            ).bind(RemAutoTemplFunctionId);
    }

    // Matcher for auto containing classes
    DeclarationMatcher makeAutoTemplateClassDeclRemMatcher() {
        return
            classTemplateDecl(
                isExpansionInMainFile(),
                hasDescendant( autoType() )
            ).bind(RemAutoTemplClassId);
    }

    // Feature Finder Matcher
    DeclarationMatcher findAutoFunctionDeclMatcher() {
        return
            decl(
                anyOf(
                    makeAutoFuncDeclRemMatcher(),
                    makeAutoTemplateFuncDeclRemMatcher(),
                    makeAutoTemplateClassDeclRemMatcher()
                )
            ).bind(FoundDeclID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_REMOVE_AUTO_MATCHERS_H
