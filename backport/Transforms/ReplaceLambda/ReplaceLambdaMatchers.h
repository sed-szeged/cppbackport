#ifndef BACKPORT_REPLACE_LAMBDA_MATCHERS_H
#define BACKPORT_REPLACE_LAMBDA_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *LambdaExprId = "lambda_expr";
const char *LambdaThisCapture = "lambda_this_expr";
const char *LambdaAncestor = "lambda_ancestor";

namespace clang { namespace ast_matchers {

    // Matcher for lambda expressions
    StatementMatcher makeLambdaExprMatcher() {
        return
            lambdaExpr(
                unless( lambdaExpr(isInTemplateInstantiation()) ),
                anyOf(
                    hasDescendant( thisExpr().bind(LambdaThisCapture) ),
                    unless( hasDescendant(thisExpr()) )
                ),
                anyOf(
                    hasAncestor( lambdaExpr().bind(LambdaAncestor) ),
                    unless( hasAncestor(lambdaExpr()) )
                )
            ).bind(LambdaExprId);
    }

    // Feature Finder Matcher
    StatementMatcher findLambdaExprStmtMatcher() {
        return
            stmt(
                makeLambdaExprMatcher()
            ).bind(FoundStmtID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_REPLACE_LAMBDA_MATCHERS_H
