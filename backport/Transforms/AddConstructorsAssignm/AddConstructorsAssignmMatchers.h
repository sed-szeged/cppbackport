#ifndef BACKPORT_ADD_CONSTRUCTORS_ASSIGNM_MATCHERS_H
#define BACKPORT_ADD_CONSTRUCTORS_ASSIGNM_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"

const char *InsertDefaultMoveAssignmentOpId = "insert_default_move_assignment_op_id";
const char *InsertDefaultCopyAssignmentOpId = "insert_default_copy_assignment_op_id";

const char *InsertImplicitDefaultConstructorsId = "insert_implicit_default_constructors_id";
const char *InsertImplicitCopyConstructorsId = "insert_implicit_copy_constructors_id";
const char *InsertImplicitMoveConstructorsId = "insert_implicit_move_constructors_id";

namespace clang { namespace ast_matchers {

        DeclarationMatcher makeInsertImplicitDefaultConstructorsMatcher() {
            return
                recordDecl(
                    hasDescendant(
                        constructorDecl(
                            isDefaultConstr(),
                            isImplicit(),
                            unless(isDeleted())
                        )
                    ),
                    anyOf(
                        hasDescendant(
                            methodDecl(isMoveAssign(), isUsed())
                        ),
                        hasDescendant(
                            constructorDecl(isMoveConstr(), isUsed())
                        )
                    )
                ).bind(InsertImplicitDefaultConstructorsId);
        }

        DeclarationMatcher makeInsertImplicitCopyConstructorsMatcher() {
            return
                recordDecl(
                    hasDescendant(
                        constructorDecl(
                            isCopyConstr(),
                            isImplicit(),
                            unless(isDeleted())
                        )
                    ),
                    anyOf(
                        hasDescendant(
                            methodDecl(isMoveAssign(), isUsed())
                        ),
                        hasDescendant(
                            constructorDecl(isMoveConstr(), isUsed())
                        )
                    )
                ).bind(InsertImplicitCopyConstructorsId);
        }

        DeclarationMatcher makeInsertImplicitMoveConstructorsMatcher() {
            return
                recordDecl(
                    hasDescendant(
                        constructorDecl(isMoveConstr(), isUsed(), isImplicit(), unless(isDeleted()))
                    )
                ).bind(InsertImplicitMoveConstructorsId);
        }

        DeclarationMatcher makeInsertDefaultMoveAssignmentOpMatcher() {
            return
                recordDecl(
                    anyOf(
                        hasDescendant(
                            methodDecl(isMoveAssign(), isUsed(), isImplicit(), unless(isDeleted()))
                        ),
                        allOf(
                            anyOf(
                                hasDescendant(
                                    methodDecl(isMoveAssign(), isUsed())
                                ),
                                hasDescendant(
                                    constructorDecl(isMoveConstr(), isUsed())
                                ),
                                makeInsertImplicitMoveConstructorsMatcher()
                            ),
                            unless(
                                hasDescendant(
                                    methodDecl(isMoveAssign(), isUserProvided())
                                )
                            )
                        )
                    )
                ).bind(InsertDefaultMoveAssignmentOpId);
        }

        DeclarationMatcher makeInsertDefaultCopyAssignmentOpMatcher() {
            return
                recordDecl(
                    anyOf(
                        hasDescendant(
                            methodDecl(isCopyAssign(), isUsed(), isImplicit(), unless(isDeleted()))
                        ),
                        allOf(
                            anyOf(
                                hasDescendant(
                                    methodDecl(isMoveAssign(), isUsed())
                                ),
                                hasDescendant(
                                    methodDecl(isMoveConstr(), isUsed())
                                ),
                                makeInsertImplicitMoveConstructorsMatcher()
                            ),
                            unless(
                                hasDescendant(
                                    methodDecl(isCopyAssign(), isUserProvided())
                                )
                            )
                        )
                    )
                ).bind(InsertDefaultCopyAssignmentOpId);
        }


        // Feature Finder Matcher
        DeclarationMatcher findGenCtrsDeclMatcher() {
            return
                decl(
                    anyOf(
                        makeInsertDefaultMoveAssignmentOpMatcher(),
                        makeInsertDefaultCopyAssignmentOpMatcher(),
                        makeInsertImplicitDefaultConstructorsMatcher(),
                        makeInsertImplicitCopyConstructorsMatcher(),
                        makeInsertImplicitMoveConstructorsMatcher()
                    )
                ).bind(FoundDeclID);
        }


} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_ADD_CONSTRUCTORS_ASSIGNM_MATCHERS_H
