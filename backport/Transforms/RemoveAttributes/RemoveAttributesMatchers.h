#ifndef BACKPORT_REMOVE_ATTRIBUTES_MATCHERS_H
#define BACKPORT_REMOVE_ATTRIBUTES_MATCHERS_H

#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *RemoveAttributesMatcherId = "remove_attributes_decl";

namespace clang { namespace ast_matchers {

        // Matches final override specifiers.
        DeclarationMatcher makeRemoveAttributesMatcher() {
            return
                decl(
                    hasAnyAttr()
                ).bind(RemoveAttributesMatcherId);
        }

        // Feature Finder Matcher
        DeclarationMatcher findRemoveAttributesDeclMatcher() {
            return
                decl(
                    makeRemoveAttributesMatcher()
                ).bind(FoundDeclID);
        }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_REMOVE_ATTRIBUTES_MATCHERS_H
