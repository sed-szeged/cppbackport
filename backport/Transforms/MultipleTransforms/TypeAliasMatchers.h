#ifndef BACKPORT_TYPE_ALIAS_MATCHERS_H
#define BACKPORT_TYPE_ALIAS_MATCHERS_H

#include "Util/CustomMatchers.h"

const char *TypedefDeclId = "typedef_decl";
const char *TypeAliasDeclId = "type_alias_decl";
const char *AliasedTypeVariableDeclId = "aliased_type_variable_decl";
const char *TypeAliasTemplateDeclId = "type_alias_template_decl";
const char *AliasedFunctionReturnTypeDeclId = "aliased_function_ret_type_decl";
const char *ContainedExprId = "expr_stmt";
const char *ClassTemplateSpecializationDeclId = "class_template_specialization_decl";
const char *FunctionTemplateSpecializationDeclId = "function_template_specialization_decl";
const char *AliasedTypeScopeResolveNameId = "type_scope_decl";

namespace clang { namespace ast_matchers {

    /**
    * Matches template type alias declarations in typedefs.
    */
    DeclarationMatcher makeTypeAliasInTypedefDeclMatcher() {
        return
            typedefDecl(
                unless(isInstantiated()),
                typedefDeclTypeContainsTemplateTypeAlias()
            ).bind(TypedefDeclId);
    }

    /**
    * Matches type alias declarations for ex.: using MyInt = int;
    */
    DeclarationMatcher makeTypeAliasDeclMatcher() {
        return
            typeAliasDecl(
                unless(hasParent(typeAliasTemplateDecl())),
                unless(isInstantiated())
            ).bind(TypeAliasDeclId);
    }

    /**
    * Matches template type alias declarations ex.: template<class T> using ptr = T*;
    */
    DeclarationMatcher makeTypeAliasTemplateDeclMatcher() {
        return
            typeAliasTemplateDecl(
                unless(isInstantiated())
            ).bind(TypeAliasTemplateDeclId);
    }

    /**
    * Matches variable declarations with types from template type alias declaration.
    */
    DeclarationMatcher makeVariableWithTypeTemplateAliasDeclMatcher() {
        return
            valueDecl(
                unless(hasAncestor(typedefDecl())),
                unless(hasAncestor(typeAliasDecl())),
                anyOf(
                    fieldDecl(unless(isInstantiated())),
                    nonTypeTemplateParmDecl(),
                    varDecl(
                        unless(isInInstantiation()),
                        unless(
                            parmVarDecl( // parmvarDecls can be in default template arguments
                                anyOf(
                                    hasAncestor(parmVarDecl()),
                                    hasAncestor(templateTypeParmDecl()),
                                    hasAncestor(nonTypeTemplateParmDecl())
                                )
                            )
                        )
                    )
                ),
                valueDeclTypeContainsTemplateTypeAlias()
            ).bind(AliasedTypeVariableDeclId);
    }

    /**
    * Matches functions.
    */
    DeclarationMatcher makeFunctionReturnTypeTemplateAliasDeclMatcher() {
        return
            functionDecl(
                unless(isInstantiated()),
                returnTypeContainsTypeAlias()
            ).bind(AliasedFunctionReturnTypeDeclId);
    }

    /**
    * Matched template default arguments with template type alias.
    */
    DeclarationMatcher makeTemplateArgumentTemplateAliasDeclMatcher() {
        return 
            templateTypeParmDecl(
                unless(isInstantiated()),
                hasDefaultArgument(),
                templateParmDeclcontainsTypeAlias()
            ).bind(AliasedTypeVariableDeclId);
    }

    /**
    * Matches type alias declarations within expressions.
    */
    StatementMatcher makeTemplateTypeAliasContainedExprMatcher() {
        return
            expr(
                exprContainsTypeAlias(),
                unless(isInTemplateInstantiation()),
                unless(newExpr(hasAncestor(newExpr()))),
                anyOf(
                    explicitCastExpr(),
                    temporaryObjectExpr(),
                    unresolvedConstructExpr(),
                    newExpr(),
                    unaryExprOrTypeTraitExpr()
                )
            ).bind(ContainedExprId);
    }

    /**
    * Matches type alias declarations within explicit class template specializations.
    */
    DeclarationMatcher makeClassTemplateSpecializationDeclMatcher() {
        return
            classTemplateSpecializationDecl(
                isExplicitTemplateSpecialization(),
                classTemplatespecializationContainsTemplateTypeAlias()
            ).bind(ClassTemplateSpecializationDeclId);
    }

    /**
    * Matches type alias declarations within explicit function template specializations.
    */
    DeclarationMatcher makeFunctionTemplateSpecializationDeclMatcher() {
        return
            functionDecl(
                isExplicitTemplateSpecialization(),
                functionDeclContainsTypeAlias()
            ).bind(FunctionTemplateSpecializationDeclId);
    }

    /**
    * Matches type alias declarations within type qualifiers.
    */
    DeclarationMatcher makeClassMethodNameTemplateTypeAliasDeclMatcher() {
        return
        methodDecl(
            unless( isInstantiated() ),
            isOutOfLine(),
            methodNameContainsTypeAlias()
        ).bind(AliasedTypeScopeResolveNameId);
    }

    // Feature Finder Matcher
    DeclarationMatcher findTypeAliasDeclMatcher() {
        return
            decl(
                anyOf(
                    makeTypeAliasInTypedefDeclMatcher(),
                    makeTypeAliasDeclMatcher(),
                    makeTypeAliasTemplateDeclMatcher(),
                    makeVariableWithTypeTemplateAliasDeclMatcher(),
                    makeFunctionReturnTypeTemplateAliasDeclMatcher(),
                    makeTemplateArgumentTemplateAliasDeclMatcher(),
                    makeClassTemplateSpecializationDeclMatcher(),
                    makeClassMethodNameTemplateTypeAliasDeclMatcher()
                )
            ).bind(FoundDeclID);
    }

    StatementMatcher findTypeAliasStmtMatcher() {
        return
            stmt(
                makeTemplateTypeAliasContainedExprMatcher()
            ).bind(FoundStmtID);
    }
} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_TYPE_ALIAS_MATCHERS_H
