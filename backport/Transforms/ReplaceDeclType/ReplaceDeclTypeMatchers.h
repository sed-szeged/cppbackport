#ifndef BACKPORT_REPLACE_DECLTYPE_MATCHERS_H
#define BACKPORT_REPLACE_DECLTYPE_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *declTypeInDeclarator = "replace_decltype_in_declarator_id";
const char *declTypeInTemplateArgument = "replace_decltype_in_template_argument_id";
const char *declTypeInStatement = "replace_decltype_in_statement_id";
const char *declTypeInTypedef = "replace_decltype_in_typedef_id";
const char *declTypeInTemplateParamDecl = "replace_decltype_in_template_paramdecl_id";

namespace clang { namespace ast_matchers {

    DeclarationMatcher makeDeclTypeInTemplateTypeParmDeclMacher() {
        return namedDecl(
                    templateTypeParmDecl(
                        hasDefaultArgument(
                            doesThisQualTypeContainsADecltype()
                        )
                    )
                ).bind(declTypeInTemplateParamDecl);
    }

    DeclarationMatcher makeDeclTypeInTypedefMatcher() {
        return
            typedefDecl(
                unless( hasAncestor(functionTemplateDecl(hasNoSpecializations())) ),
                unless( hasAncestor(classTemplateDecl(hasNoSpecializations())) ),
                hasUnderlyingType(
                    doesThisQualTypeContainsADecltype()
                )
            ).bind(declTypeInTypedef);
    }

    DeclarationMatcher makeDecltypeInTemplateArgumentMatcher() {
        return
            declaratorDecl(
                unless(hasAncestor(typedefDecl())),
                unless( hasAncestor(functionTemplateDecl(hasNoSpecializations())) ),
                unless( hasAncestor(classTemplateDecl(hasNoSpecializations())) ),
                anyOf(
                    hasType(
                        templateSpecializationType(
                            hasAnyTemplateArgument(
                                refersToType(
                                    doesThisQualTypeContainsADecltype()
                                )
                            )
                        )
                    ),
                    hasType(
                        elaboratedType(
                            namesType(
                                hasDeclaration(
                                    decl(
                                        classTemplateSpecializationDecl(
                                            hasAnyTemplateArgument(
                                                refersToType(
                                                    doesThisQualTypeContainsADecltype()
                                                )
                                            )
                                        )
                                    )
                                )
                            )
                        )
                    )
                )
                
            ).bind(declTypeInTemplateArgument);
    }

    DeclarationMatcher makeDecltypeInDeclarationMatcher() {
        return
            declaratorDecl(
                unless( hasAncestor(typedefDecl()) ),
                unless( parmVarDecl(hasAncestor(varDecl())) ),
                unless( hasAncestor(functionTemplateDecl(hasNoSpecializations())) ),
                unless( hasAncestor(classTemplateDecl(hasNoSpecializations())) ),
                unless( functionDecl(unless(valueDecl(hasType(isThisADecltype())))) ),
                unless( functionTemplateDecl(unless(valueDecl(hasType(isThisADecltype())))) ),
                hasType(
                    doesThisQualTypeContainsADecltype()
                )
            ).bind(declTypeInDeclarator);
    }

    StatementMatcher makeDecltypeInStatementMatcher() {
        return
            expr(
                unless( hasAncestor(functionTemplateDecl(hasNoSpecializations())) ),
                unless( hasAncestor(classTemplateDecl(hasNoSpecializations())) ),
                anyOf(
                    allOf(
                        hasType(
                            doesThisQualTypeContainsADecltype()
                        ),
                        unless( callExpr() )
                    ),
                    unaryExprOrTypeTraitExpr(
                        hasArgumentOfType(
                            doesThisQualTypeContainsADecltype()
                        )
                    ),
                    explicitCastExpr(
                        hasDestinationType(
                            doesThisQualTypeContainsADecltype()
                        )
                    )
                )
            ).bind(declTypeInStatement);
    }

    // Feature Finder Matchers
    DeclarationMatcher findReplaceDecltypeInDeclMatcher() {
        return
            decl(
                anyOf(
                    makeDecltypeInDeclarationMatcher(),
                    makeDecltypeInTemplateArgumentMatcher(),
                    makeDeclTypeInTypedefMatcher(),
                    makeDeclTypeInTemplateTypeParmDeclMacher()
                )
            ).bind(FoundDeclID);
    }

    StatementMatcher findReplaceDecltypeInStmtMatcher() {
        return
            stmt(
                makeDecltypeInStatementMatcher() 
            ).bind(FoundStmtID);
    }


} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_REPLACE_DECLTYPE_MATCHERS_H
