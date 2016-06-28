#ifndef BACKPORT_DELEGATING_CONSTRUCTOR_MATCHERS_H
#define BACKPORT_DELEGATING_CONSTRUCTOR_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *DelegatingConstructorMatcherId = "delegating_constructor_decl";

namespace clang { namespace ast_matchers {

    DeclarationMatcher makeDelegatingConstructorMatcher() {
        return
            constructorDecl(
                unless( hasAncestor(functionTemplateDecl(hasNoSpecializations())) ),
                unless( hasAncestor(classTemplateDecl(hasNoSpecializations())) ),
                isUserProvided(),
                isDelegatingConstructor()
            ).bind(DelegatingConstructorMatcherId);
    }

    DeclarationMatcher findDelegatingConstructorDeclMatcher() {
        return
            decl(
                makeDelegatingConstructorMatcher()
            ).bind(FoundDeclID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_DELEGATING_CONSTRUCTOR_MATCHERS_H
