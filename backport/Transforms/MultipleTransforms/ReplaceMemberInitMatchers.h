#ifndef BACKPORT_REPLACE_MEMBER_INIT_MATCHERS_H
#define BACKPORT_REPLACE_MEMBER_INIT_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *CtorDeclId = "ctor_decl";
const char *ClassTemplateDeclId = "class_template_decl";
const char *ClassDeclId = "class_decl";

namespace clang { namespace ast_matchers {

    /**
      * \brief Creates a matcher that matches user provided constructors with in-class initialization.
      */
    DeclarationMatcher makeConstructorInClassMemberInitMatcher() {
        return
            constructorDecl(
                anyOf(
                    hasAncestor( classTemplateDecl().bind(ClassTemplateDeclId) ),
                    unless ( hasAncestor(classTemplateDecl()) )
                    //unless( hasAncestor( classTemplateSpecializationDecl() ) )
                ),
                isUserProvided(),
                hasAnyConstructorInitializer( isInClassInitializer() )
            ).bind(CtorDeclId);
    }

    /**
      * \brief Create a matcher which matches class declarations with in-class initializations.
      */
    DeclarationMatcher makeRecordDeclMatcherWithInClassInit() {
        return
            recordDecl(
                unless( isTemplateInstantiation() ),
                hasInClassInitializer()
                ).bind(ClassDeclId);
    }

    /**
      * \brief Creates a matcher which matches classes without declared constructors.
      */
    DeclarationMatcher makeClassInClassMemberInitWithoutCtorMatcher() {
        return
            recordDecl(
                unless( isTemplateInstantiation() ),
                unless( hasDeclaredCtor() ),
                hasInClassInitializer()
            ).bind(ClassDeclId);
    }

    // Feature Finder Matcher
    DeclarationMatcher findReplaceMemberInitDeclMatcher() {
        return
            decl(
                anyOf(
                    makeConstructorInClassMemberInitMatcher(),
                    makeRecordDeclMatcherWithInClassInit(),
                    makeClassInClassMemberInitWithoutCtorMatcher()
                )
            ).bind(FoundDeclID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_REPLACE_MEMBER_INIT_MATCHERS_H
