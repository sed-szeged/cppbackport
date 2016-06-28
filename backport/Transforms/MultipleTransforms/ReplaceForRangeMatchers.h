#ifndef BACKPORT_REPLACE_FOR_RANGE_MATCHERS_H
#define BACKPORT_REPLACE_FOR_RANGE_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *ForRangeStmtID = "for_range_stmt";

namespace clang { namespace ast_matchers {

    // Matcher for for-range statements
    StatementMatcher makeForRangeStmtMatcher() {
        return
            forRangeStmt(
                isExpansionInMainFile(),
                unless( isInTemplateInstantiation() )
            ).bind(ForRangeStmtID);
    }

    // Feature Finder Matcher
    StatementMatcher findForRangeStmtMatcher() {
        return
            stmt(
                makeForRangeStmtMatcher()
            ).bind(FoundStmtID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_REPLACE_FOR_RANGE_MATCHERS_H
