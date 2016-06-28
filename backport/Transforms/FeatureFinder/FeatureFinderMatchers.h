#ifndef BACKPORT_FEATURE_FINDER_MATCHERS_H
#define BACKPORT_FEATURE_FINDER_MATCHERS_H

#include "Util/CustomMatchers.h"

extern const char GenConstructorsID[];
extern const char ModifyFunctionsID[];
extern const char InstantiateTemplateID[];
extern const char ReplaceDeclTypeID[];
extern const char ReplaceLambdaID[];
extern const char ReplaceRvalueID[];
extern const char MultipleTransformsID[];
extern const char RemoveAutoDelegationID[];

extern const char *FoundStmtID;
extern const char *FoundDeclID;

namespace clang { namespace ast_matchers {

    // Gen ctors
    extern DeclarationMatcher findGenCtrsDeclMatcher();

    // Modify Variadic
    extern DeclarationMatcher findModifyVariadicDeclMatcher();
    extern StatementMatcher findModifyVariadicStmtMatcher();

    // Modify Auto
    extern DeclarationMatcher findModifyAutoDeclMatcher();
    extern StatementMatcher findModifyAutoStmtMatcher();

    // Instantiate Template
    extern DeclarationMatcher findInstantiateTemplateDeclMatcher();

    // Replace decltype
    extern DeclarationMatcher findReplaceDecltypeInDeclMatcher();
    extern StatementMatcher findReplaceDecltypeInStmtMatcher();

    // Replace Member Init
    extern DeclarationMatcher findReplaceMemberInitDeclMatcher();

    // Replace Lambda
    extern StatementMatcher findLambdaExprStmtMatcher();

    // Replace For Range
    extern StatementMatcher findForRangeStmtMatcher();

    // Replace Rvalue
    extern DeclarationMatcher findReplaceRvalueDeclMatcher();
    extern StatementMatcher findReplaceRvalueStmtMatcher();

    // Deduce Auto
    extern DeclarationMatcher findDeduceAutoDeclMatcher();
    extern StatementMatcher findDeduceAutoStmtMatcher();

    // type alias
    extern DeclarationMatcher findTypeAliasDeclMatcher();
    extern StatementMatcher findTypeAliasStmtMatcher();

    // Remove Auto
    extern DeclarationMatcher findAutoFunctionDeclMatcher();

    // Delegating constructor
    extern DeclarationMatcher findDelegatingConstructorDeclMatcher();

    // Attributes matcher
    extern DeclarationMatcher findRemoveAttributesDeclMatcher();

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_FEATURE_FINDER_MATCHERS_H
