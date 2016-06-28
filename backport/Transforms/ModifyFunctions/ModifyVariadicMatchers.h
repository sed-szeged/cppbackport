#ifndef BACKPORT_MODIFY_VARIADIC_MATCHERS_H
#define BACKPORT_MODIFY_VARIADIC_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *VariadicTemplateFunctionId = "variadic_template_function";
const char *VariadicFuncCallId = "variadic_function_call";
const char *VariadicTemplateWithCallExprID = "variadic_template_with_call";
const char *CalledFuncFromVariadicFuncID = "called_func_from_variadic_func";
const char *VariadicFuncCallerID = "variadic_func_caller";
const char *NumCountVariadicTemplateWithCallExprID = "num_count_variadic_template_with_call";
const char *NumCountCalledFuncFromVariadicFuncID = "num_count_called_func_from_variadic_func";
const char *NumCountFuncCallerID = "num_count_func_caller";

namespace clang { namespace ast_matchers {

    // Matcher for variadic template functions
    DeclarationMatcher makeVariadicTemplateFunctionMatcher() {
        return
            functionTemplateDecl(
                isVariadicTemplate()
            ).bind(VariadicTemplateFunctionId);
    }

    // Matcher for the final call from a variadic function to the non variadic function (usually of the same name) that ends the recursion
    DeclarationMatcher makeVariadicFinalFunctionMatcher() {
        return
            functionTemplateDecl(
                isVariadicTemplate(),
                forEachDescendant(
                    callExpr(
                        callee(
                            decl(
                                anyOf(
                                    functionDecl(),
                                    functionTemplateDecl( unless(isVariadicTemplate()) )
                                )
                            ).bind(CalledFuncFromVariadicFuncID)
                        )
                    ).bind(VariadicFuncCallerID)
                )
            ).bind(VariadicTemplateWithCallExprID);
    }

    // Matcher that counts for each function call from a variadic function the number of arguments passed
    // This is important to determine the number of arguments that are extracted from the parameter pack
    DeclarationMatcher makeVariadicFinalFunctionVarCountMatcher() {
        return
            functionTemplateDecl(
                isVariadicTemplate(),
                forEachDescendant(
                    callExpr(
                        callee(
                            decl(
                                anyOf(
                                    functionDecl(),
                                    functionTemplateDecl( unless(isVariadicTemplate()) )
                                )
                            ).bind(NumCountCalledFuncFromVariadicFuncID)
                        )
                    ).bind(NumCountFuncCallerID)
                )
            ).bind(NumCountVariadicTemplateWithCallExprID);
    }

    // Matcher for calls to variadic functions
    StatementMatcher makeVariadicFuncCallMatcher() {
        return
            callExpr(
                callee(
                    functionDecl(
                        hasFunctionTemplate( functionTemplateDecl(isVariadicTemplate()) )
                    )
                )
            ).bind(VariadicFuncCallId);
    }

    // Feature Finder Matcher
    DeclarationMatcher findModifyVariadicDeclMatcher() {
        return
            decl(
                makeVariadicTemplateFunctionMatcher()
            ).bind(FoundDeclID);
    }

    StatementMatcher findModifyVariadicStmtMatcher() {
        return
            stmt(
                makeVariadicFuncCallMatcher()
            ).bind(FoundStmtID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_MODIFY_VARIADIC_MATCHERS_H
