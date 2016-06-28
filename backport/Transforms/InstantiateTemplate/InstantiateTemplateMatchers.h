#ifndef BACKPORT_INSTANTIATE_TEMPLATE_MATCHERS_H
#define BACKPORT_INSTANTIATE_TEMPLATE_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *TemplateFunctionId = "template_function";
const char *TemplateClassId = "template_class";

namespace clang { namespace ast_matchers {

    // Matcher for template functions that need to be instantiated because they contain template dependent auto or decltype expression
    DeclarationMatcher makeTemplateFunctionMatcher() {
        return
            functionTemplateDecl(
                unless( hasNoSpecializations() ),
                unless(
                    functionTemplateDecl(
                        hasTrailingReturnType(),
                        hasDependentReturnType()
                    )
                ),
                isExpansionInMainFile(),
                anyOf(
                    hasDescendant(
                        varDecl(
                            isDependentVarDecl(),
                            hasDescendant(autoType())
                        )
                    ),
                    hasDescendant(
                        varDecl(
                            isDependentVarDecl(),
                            hasType(doesThisQualTypeContainsADecltype())
                        )
                    ),
                    hasDescendant(
                        stmt(
                            findReplaceDecltypeInStmtMatcher()
                        )
                    ),
                    hasDescendant(
                        decl(
                            findReplaceDecltypeInDeclMatcher()
                        )
                    ),
                    hasForwardingReferenceParam()
                ),
                unless( functionTemplateDecl(isVariadicTemplate()) )
            ).bind(TemplateFunctionId);
    }

    // Matcher for template classes that need to be instantiated because they contain template dependent auto or decltype expression
    DeclarationMatcher makeTemplateClassMatcher() {
        return
            classTemplateDecl(
                unless(hasNoSpecializations()),
                isExpansionInMainFile(),
                anyOf(
                    hasDescendant(
                        functionDecl(
                            hasTrailingReturnType(),
                            hasDependentReturnType()
                        )
                    ),
                    hasDescendant(
                        varDecl(
                            isDependentVarDecl(),
                            hasDescendant(autoType())
                        )
                    ),

                    hasDescendant(
                        functionDecl(
                            anyOf(
                                allOf(
                                    hasTrailingReturnType(),
                                    hasDependentReturnType()
                                ),
                                hasDescendant(
                                    declaratorDecl(
                                        hasType(doesThisQualTypeContainsADecltype())
                                    )
                                )
                            )
                        )
                    ),
                    hasDescendant(
                        declaratorDecl(
                            anyOf(
                                hasType(doesThisQualTypeContainsADecltype()),
                                hasDescendant(
                                    declaratorDecl(hasType(doesThisQualTypeContainsADecltype()))
                                ),
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
                            )
                        )
                    ),
                    hasDescendant(
                        stmt(
                            findReplaceDecltypeInStmtMatcher()
                        )
                    ),
                    hasDescendant(
                        decl(
                            findReplaceDecltypeInDeclMatcher(),
                            unless(namedDecl(templateTypeParmDecl(unless(hasDefaultArgument(isDependentQualType())))))
                        )
                    )/*,
                    hasDescendant(
                        decl(
                            findGenCtrsDeclMatcher()
                        )
                    ),
                    has(
                        classTemplateSpecializationDecl(
                            has(
                                recordDecl( unless(isImplicit() ) ) 
                            ),

                                
                            hasDescendant(
                                decl(
                                    anyOf(
                                        decl(
                                            findReplaceRvalueDeclMatcher()
                                        ),

                                        decl(
                                            hasDescendant(
                                                findReplaceRvalueStmtMatcher()
                                            )
                                        )
                                    
                                    )
                                )
                            )
                        )
                    )*/
                ),
                unless( classTemplateDecl(isVariadicTemplate()) )
            ).bind(TemplateClassId);
    }

    // Feature Finder Matcher
    DeclarationMatcher findInstantiateTemplateDeclMatcher() {
        return
            decl(
                anyOf(
                    makeTemplateFunctionMatcher(),
                    makeTemplateClassMatcher()
                )
            ).bind(FoundDeclID);
    }

} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_INSTANTIATE_TEMPLATE_MATCHERS_H
