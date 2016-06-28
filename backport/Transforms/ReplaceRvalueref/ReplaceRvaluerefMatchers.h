#ifndef BACKPORT_REPLACE_RVALUE_REF_MATCHERS_H
#define BACKPORT_REPLACE_RVALUE_REF_MATCHERS_H

#include "Util/CustomMatchers.h"
#include "Transforms/FeatureFinder/FeatureFinderMatchers.h"
#include "Util/MoveMacros.h"

const char *GlobalInsertDeclId = "start_up_decl_id";

const char *RvalueRefId = "rvalue_ref_id";
const char *LvalueRefInCopyAssignId = "lvalue_in_copy_assign_ref_id";
const char *StdMoveId = "std_move_id";

const char *InsertCopyableAndMoveableId = "insert_copyable_and_moveable_id";
const char *InsertNotCopyableAndMoveableId = "insert_not_copyable_and_moveable_id";
const char *InsertCopyableAndNotMoveableId = "insert_copyable_and_not_moveable_id";

const char *insertReturnValueMacro = "insert_return_value_macro_id";
const char *moveTemporaryWhenICan = "move_temporary_when_i_can_id";
const char *implicitCastXvalue = "implicit_cast_xvalue_id";

const char *rvalue_cast = "rvalue_cast_id";

const char *rvlaue_ref_in_vardecl_not_paramdecl = "rvalue_ref_in_vardecl_not_paramdecl";

const char *replace_move_initialization = "replace_move_initialization_id";

const char *replace_rvalue_in_template_def_val = "replace_rvalue_in_template_def_val_id";
const char *replace_rvalue_in_typedef = "replace_rvalue_in_typedef_id";

const char *replace_rvalue_in_declarator_type = "replace_rvalue_in_declarator_type_id";
const char *replace_rvalue_in_template_specialization = "replace_rvalue_in_template_specialization_id";
const char *replace_rvalue_in_constructExpr = "replace_rvalue_in_constructExpr_id";

const char *replace_rvalue_in_decl_through_refexpr = "replace_rvalue_in_decl_through_refexpr_id";


std::string backport_rv(std::string name_of_namespace) {
    return "\n/*1000000001*/\n" + getMoveMacro(one_time_include);
}

namespace clang { namespace ast_matchers {
        DeclarationMatcher makeRvalueInTemplateSpecializationParameterMatcher() {
            return classTemplateSpecializationDecl(
                    hasAnyTemplateArgument(
                        hasTypeTemplateArg(
                            anyOf(
                                rValueReferenceType(),
                                hasSubTypeMatcher(
                                    rValueReferenceType()
                                )
                            )
                        )
                    )
                ).bind(replace_rvalue_in_template_specialization);
        }

        StatementMatcher makeRvalueInConstrExprMatcher() {
            return constructExpr(
                    hasType(
                        hasSubTypeMatcher(
                            rValueReferenceType()
                        )
                    )
                ).bind(replace_rvalue_in_constructExpr);
        }

        StatementMatcher makeimplicitCastXvalue() {
            return implicitCastExpr(
                    has(expr(isXValue())),
                    unless(
                        hasAncestor(
                            decl(
                                isImplicit()
                            )
                        )
                    )
                ).bind(implicitCastXvalue);
        }

        StatementMatcher makeMoveInitializationMatcher() {
            return
                constructExpr(
                    hasDeclaration(
                        constructorDecl(
                            isMoveConstr(),
                            isUserProvided()
                        )
                    ),
                    anyOf(
                        hasParent(
                            varDecl()
                        ),
                        hasParent(
                            exprWithCleanups(
                                hasParent(
                                    varDecl()
                                )
                            )
                        )
                    )
                ).bind(replace_move_initialization);
        }

        DeclarationMatcher makeRvalueMatcher() {
            return
                functionDecl(
                    anyOf(
                        hasAnyParameter( hasType( qualType( anyOf(rValueReferenceType(), hasSubTypeMatcher(rValueReferenceType()) ) ) ) ),
                        returns(
                            anyOf(
                                qualType( anyOf(rValueReferenceType(), hasSubTypeMatcher(rValueReferenceType()) ) ) ,
                                hasCanonicalType( // bc of typedef
                                    qualType( anyOf(rValueReferenceType(), hasSubTypeMatcher(rValueReferenceType()) ) ) 
                                )
                            )
                        )
                    ),
                    unless( functionDecl( isImplicit() ) )
                ).bind(RvalueRefId);
        }

        StatementMatcher makeMoveStatementMatcher() {
            return
                callExpr(
                    argumentCountIs(1),
                    callee( functionDecl( hasName("::std::move") ) ),
                    unless(hasParent(implicitCastExpr())) // handle this case separatly.
                ).bind(StdMoveId);
        }

        DeclarationMatcher makeCopyableAndMoveableMatcher() {
            return recordDecl(
                has( constructorDecl( isCopyConstr(), isPublic(), unless( isDeleted() ) ) ),
                has(methodDecl(isMoveAssign(), isPublic(), isUserProvided(), unless(isDeleted())))
            ).bind(InsertCopyableAndMoveableId);
        }

        DeclarationMatcher makeNotCopyableAndMoveableMatcher() {
            return recordDecl(
                anyOf(
                    unless(has(constructorDecl(isCopyConstr(), isPublic(), unless(isDeleted())))), // No copy constr.
                    has(constructorDecl(isCopyConstr(), isDeleted())) //Copy Constr is deleted.
                ),
                has(methodDecl(isMoveAssign(), isPublic(), isUserProvided(), unless(isDeleted())))
            ).bind(InsertNotCopyableAndMoveableId);
        }

        DeclarationMatcher makeCopyableAndNotMoveableMatcher() {
            return recordDecl(
                has(constructorDecl(isCopyConstr(), isPublic(), unless(isDeleted()))),
                anyOf(
                    unless(has(methodDecl(isMoveAssign(), isPublic(), isUserProvided()))),   // No move assign.
                    has(methodDecl(isMoveAssign(), isDeleted())) // Move assign is deleted.
                )
            ).bind(InsertCopyableAndNotMoveableId);
        }

            DeclarationMatcher makeLvalueInCopyAssignMatcher() {
            return
                methodDecl(
                    isCopyAssign(),
                    unless( functionDecl( isImplicit() ) ),
                    hasParent(
                        recordDecl(
                            makeCopyableAndMoveableMatcher()
                        )
                    )
                ).bind(LvalueRefInCopyAssignId);
        }

        DeclarationMatcher makeDeclMatcher() {
            return
                decl(
                    hasParent(
                        isTranslationUnit()
                    )
                ).bind(GlobalInsertDeclId);
        }

        StatementMatcher makeReturnValueMatcher() {
            return
                returnStmt(
                    hasDescendant(
                        implicitCastExpr(
                            isXValue()
                        )
                    ),
                    retValExpr( hasTypeWithUserDeclaredMoveConstr() ),
                    retValExpr( hasNonTrivialCXXRecordType() )
                ).bind(insertReturnValueMacro); 
        }

        StatementMatcher makeRvalueCastMatcher() {
            return
                explicitCastExpr(
                    hasDestinationType(
                        anyOf(
                            qualType( anyOf(rValueReferenceType(), hasSubTypeMatcher(rValueReferenceType()) ) ) ,
                            hasCanonicalType( // bc of typedef
                                qualType( anyOf(rValueReferenceType(), hasSubTypeMatcher(rValueReferenceType()) ) ) 
                            )
                        ) 
                    ),
                    unless(
                        hasAncestor(
                            decl(
                                isImplicit()
                            )
                        )
                    )
                ).bind(rvalue_cast);
        }

        DeclarationMatcher makeRvalueVarDeclNotParamDeclMatcher() {
            return
                varDecl(
                    unless(parmVarDecl()),
                    unless(isImplicit()),
                    hasType(
                        qualType(
                            anyOf(
                                qualType( rValueReferenceType() ) ,
                                hasCanonicalType( // bc of typedef
                                    qualType( rValueReferenceType() ) 
                                )
                            )
                        )
                    )
                ).bind(rvlaue_ref_in_vardecl_not_paramdecl);
        }


        DeclarationMatcher makeRvalueInDeclaratorMatcher() {
            return declaratorDecl(
                    hasType(
                        hasSubTypeMatcher(
                            rValueReferenceType()
                        )
                    ),
                    unless(functionDecl()),
                    unless(parmVarDecl()),
                    unless(varDecl(hasType(rValueReferenceType()))),
                    unless(declaratorDecl(isImplicit())),
                    unless(varDecl(isImplicit())),
                    unless(declaratorDecl(hasAncestor(functionDecl(isImplicit()))))
                ).bind(replace_rvalue_in_declarator_type);
        }

        StatementMatcher makeTemporaryToXValueMatcher() {
            return
                temporaryObjectExpr(
                    hasAncestor( materializeTemporaryExpr(
                        isXValue()
                    ) ),
                    anyOf(
                        allOf(
                            hasTypeWithUserDeclaredMoveConstr(),
                            hasNonTrivialCXXRecordType()
                        ),
                        unless(
                            hasType(
                                recordType()
                            )
                        )
                    ),

                    unless(
                        hasAncestor(
                            returnStmt() // handle the implicit move form function separately.
                        )
                    )
                    
                ).bind(moveTemporaryWhenICan);
        }

        DeclarationMatcher makeRvalueRefInTemplateTypeParmDeclMacher() {
            return namedDecl(
                        templateTypeParmDecl(
                            hasDefaultArgument(
                                qualType( anyOf(rValueReferenceType(), hasSubTypeMatcher(rValueReferenceType()) ) ) 
                            )
                        )
                    ).bind(replace_rvalue_in_template_def_val);
        }

        
        DeclarationMatcher makeRvalueRefInTypedefMatcher() {
            return
                typedefDecl(
                    hasUnderlyingType(
                        qualType( anyOf(rValueReferenceType(), hasSubTypeMatcher(rValueReferenceType()) ) ) 
                    )
                ).bind(replace_rvalue_in_typedef);
        }

        StatementMatcher makeDeclRefToRvalueDeclaration() {
            return
                declRefExpr(
                    to(
                        makeRvalueInDeclaratorMatcher()
                    )
                ).bind(replace_rvalue_in_decl_through_refexpr);
        }

        // Feature Finder Matcher
        DeclarationMatcher findReplaceRvalueDeclMatcher() {
            return
                decl(
                    anyOf(
                        makeRvalueMatcher(),
                        makeLvalueInCopyAssignMatcher(),
                        makeCopyableAndMoveableMatcher(),
                        makeNotCopyableAndMoveableMatcher(),
                        makeCopyableAndNotMoveableMatcher(),
                        makeRvalueVarDeclNotParamDeclMatcher(),
                        makeRvalueRefInTemplateTypeParmDeclMacher(),
                        makeRvalueRefInTypedefMatcher(),
                        anyOf( // anyOf can only have a maximum of 9 arguments
                            makeRvalueInDeclaratorMatcher(),
                            makeRvalueInTemplateSpecializationParameterMatcher()
                        )
                    )
                ).bind(FoundDeclID);
        }

        StatementMatcher findReplaceRvalueStmtMatcher() {
            return
                stmt(
                    anyOf(
                        makeMoveStatementMatcher(),
#ifndef NO_IMPLICIT_MOVE
                        makeTemporaryToXValueMatcher(),
                        makeReturnValueMatcher(),
#endif
                        makeRvalueCastMatcher(),
                        makeMoveInitializationMatcher(),
                        makeimplicitCastXvalue(),
                        makeRvalueInConstrExprMatcher(),
                        makeDeclRefToRvalueDeclaration()
                    )
                ).bind(FoundStmtID);
        }


} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_REPLACE_RVALUE_REF_MATCHERS_H
