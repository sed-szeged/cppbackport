#ifndef BACKPORT_DEDUCE_AUTO_MATCHERS_H
#define BACKPORT_DEDUCE_AUTO_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *SingleAutoDeclId = "single_auto_decl";
const char *MultipleAutoDeclId = "multiple_auto_decl";
const char *AutoDeclWithoutDeclStmtId = "auto_decl_without_declstmt";
const char *DeclTypeMethodId = "auto_method_decltype";

namespace clang { namespace ast_matchers {

    /**
     * Matches auto declarations with single variable declaration.
     */
    StatementMatcher makeSingleAutoDeclMatcher() {
        return
            declStmt(
                unless( hasParent(forRangeStmt()) ),
                unless( hasAncestor(functionTemplateDecl(hasNoSpecializations())) ),
                unless( hasAncestor(classTemplateDecl(hasNoSpecializations())) ),
                unless( isInTemplateInstantiation() ),
                has( varDecl(unless(isImplicit())) ),
                hasSingleDecl( hasDescendant(autoType()) )
            ).bind(SingleAutoDeclId);
    }

    /**
     * Matches auto declarations with multiple variable declaration.
     */
    StatementMatcher makeMultipleAutoDeclMatcher() {
        return
            declStmt(
                unless( hasAncestor(functionTemplateDecl(hasNoSpecializations())) ),
                unless( hasAncestor(classTemplateDecl(hasNoSpecializations())) ),
                unless( isInTemplateInstantiation() ),
                unless( hasSingleDecl(anything()) ),
                has( varDecl(unless(isImplicit())) ),
                hasDescendant( autoType() )
            ).bind(MultipleAutoDeclId);
    }

    /**
     * Matches auto declarations in global scope.
     */
    DeclarationMatcher makeAutoDeclarationMatcherWithoutDeclStmt() {
        return
            varDecl(
                unless( hasAncestor(functionTemplateDecl(hasNoSpecializations())) ),
                unless( hasAncestor(classTemplateDecl(hasNoSpecializations())) ),
                unless( isInstantiated() ),
                unless( hasAncestor(declStmt()) ),
                hasDescendant( autoType() )
            ).bind(AutoDeclWithoutDeclStmtId);
    }

    /**
     * Matches functions with suffix return type.
     */
    DeclarationMatcher makeDeclTypeMethodMatcher() {
        return
            functionDecl(
                unless(
                    hasAncestor(
                        functionTemplateDecl(
                            hasTrailingReturnType(),
                            hasDependentReturnType()
                        )
                    )
                ),
                unless(
                    allOf(
                        functionDecl(
                            isTemplateSpecialization()
                        ),
                        hasFunctionTemplate(
                            functionTemplateDecl(
                                hasTrailingReturnType(),
                                hasDependentReturnType()
                            )
                        )
                    )
                ),
                unless( isTemplateInstantiation() ),
                hasTrailingReturnType()
            ).bind(DeclTypeMethodId);
    }

    // Feature Finder Matcher
    DeclarationMatcher findDeduceAutoDeclMatcher() {
        return
            decl(
                anyOf(
                    makeAutoDeclarationMatcherWithoutDeclStmt(),
                    makeDeclTypeMethodMatcher()
                )
            ).bind(FoundDeclID);
    }

    StatementMatcher findDeduceAutoStmtMatcher() {
        return
            stmt(
                anyOf(
                    makeSingleAutoDeclMatcher(),
                    makeMultipleAutoDeclMatcher()
                )
            ).bind(FoundStmtID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_DEDUCE_AUTO_MATCHERS_H
