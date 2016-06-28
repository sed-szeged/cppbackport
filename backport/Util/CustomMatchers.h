#ifndef BACKPORT_CUSTOM_MATCHERS
#define BACKPORT_CUSTOM_MATCHERS

#include "clang/ASTMatchers/ASTMatchers.h"
#include "TransformUtility.h"
#include <functional>

namespace clang { namespace ast_matchers {

/* This macro creates an AST matcher with NAME which can be called on TYPE
   and returns true if the ACTION method called on the node returns true.
   also optionally one additional criteria (the name of the node is T or Node)
   can be specified, as the last parameter. */
#define SIMPLE_MATCHER_WITH_ACTION_DECL(NAME, TYPE, ACTION, ...) \
    AST_MATCHER(TYPE, NAME) { \
        TYPE const *T = dyn_cast<TYPE>(&Node); \
        return T != nullptr && T->ACTION() && (true, ##__VA_ARGS__); \
    }

/* This macro creates an AST matcher with NAME which can be called on TYPE
   and returns true if the optional third criteria is true (the name of the node is T or Node). */
#define SIMPLE_MATCHER_DECL(NAME, TYPE, ...) \
    AST_MATCHER(TYPE, NAME) { \
        TYPE const *T = dyn_cast<TYPE>(&Node); \
        return T != nullptr && (true, ##__VA_ARGS__); \
    }

/* This macro creates an AST matcher with NAME which can be called on TYPE
   and returns true if the node is convertable to INTERNAL_TYPE and if given
   the optional fourth criteria is true (the name of the node is T(as INTERNAL_TYPE) or Node(as TYPE)). */
#define SIMPLE_MATCHER_DECL_DIFF_TYPE(NAME, TYPE, INTERNAL_TYPE, ...) \
    AST_MATCHER(TYPE, NAME) { \
        INTERNAL_TYPE const *T = dyn_cast<INTERNAL_TYPE>(&Node); \
        return T != nullptr && (true, ##__VA_ARGS__); \
    }

/* This macro creates an AST matcher with NAME which can be called on TYPE
   and returns true if the node is convertable to INTERNAL_TYPE
   and if the ACTION method called on the node as INTERNAL_TYPE is true and if given
   the optional fourth criteria is true (the name of the node is T(as INTERNAL_TYPE) or Node(as TYPE)). */
#define SIMPLE_MATCHER_WITH_ACTION_DECL_DIFF_TYPE(NAME, TYPE, INTERNAL_TYPE, ACTION, ...) \
    AST_MATCHER(TYPE, NAME) { \
        INTERNAL_TYPE const *T = dyn_cast<INTERNAL_TYPE>(&Node); \
        return T != nullptr && T->ACTION() && (true, ##__VA_ARGS__); \
    }

/* This macro creates an AST matcher with NAME which can be called on TYPE 
   and can take an other matcher which matches to PARM_TYPE nodes,
   and returns true if the node is convertable to INTERNAL_TYPE
   and if the matcher got as a parameter returns true for the dereferenced 
   result of the ACTION method called on the node as INTERNAL_TYPE and 
   the optional fourth criteria is true (the name of the node is T(as INTERNAL_TYPE) or Node(as TYPE)). */
#define SIMPLE_MATCHER_WITH_ACTION_WITH_PARAM_DECL(NAME, TYPE, INTERNAL_TYPE, PARM_TYPE, ACTION, ...) \
    AST_MATCHER_P(TYPE, NAME, internal::Matcher<PARM_TYPE>, InnerMatcher) { \
        INTERNAL_TYPE const *T = dyn_cast<INTERNAL_TYPE>(&Node); \
        return T != nullptr && InnerMatcher.matches(*(T->ACTION()), Finder, Builder) && (true, ##__VA_ARGS__); \
    }

// Matches declarations
#define AST_DECL_MATCHER(NodeType, MatcherName) \
    const internal::VariadicDynCastAllOfMatcher<Decl, NodeType> MatcherName;

    // Returns true if the template is a variadic template
    AST_POLYMORPHIC_MATCHER(hasNoSpecializations, AST_POLYMORPHIC_SUPPORTED_TYPES_2(FunctionTemplateDecl, ClassTemplateDecl)) {
        int specCount = std::distance(Node.spec_begin(), Node.spec_end());
        if (specCount == 0) {
            return true;
        } else {
            return false;
        }
    }

    // Returns true if a record contains in-class initialization.
    SIMPLE_MATCHER_DECL(hasInClassInitializer, CXXRecordDecl, T->hasDefinition() && !T->field_empty() && T->hasInClassInitializer())

    // Returns true if a record contains user declared constructor.
    SIMPLE_MATCHER_DECL(hasDeclaredCtor, CXXRecordDecl, T->hasDefinition() && T->hasUserDeclaredConstructor())

    // Returns true if constructor contains in-class initialization.
    SIMPLE_MATCHER_WITH_ACTION_DECL(isInClassInitializer, CXXCtorInitializer, isInClassMemberInitializer)

    // Returns true if the current Node is a delegating constructor
    SIMPLE_MATCHER_WITH_ACTION_DECL(isDelegatingConstructor, CXXConstructorDecl, isDelegatingConstructor)

    // Returns true if constructor is created by user.
    SIMPLE_MATCHER_WITH_ACTION_DECL(isUserProvided, CXXMethodDecl, isUserProvided)

    // Returns true on constructor definition.
    SIMPLE_MATCHER_WITH_ACTION_DECL(hasBody, CXXConstructorDecl, doesThisDeclarationHaveABody)

    // Returns true if the auto type is template dependent.
    // FIXME : This does not work for 'static const auto' class members
    SIMPLE_MATCHER_WITH_ACTION_DECL(isDependentAutoType, AutoType, getDeducedType().isNull)

    // Returns true if the VarDecl is template dependent.
    AST_MATCHER(VarDecl, isDependentVarDecl) {
        auto init = Node.getInit();
        if ((init != nullptr) && (init->isTypeDependent() || Node.getType().getCanonicalType()->isDependentType() || Node.getType().getCanonicalType()->isInstantiationDependentType())) {
            return true;
        } else {
            return false;
        }
    }

    // Returns true if the declaration has an unknown attribute
    AST_MATCHER(Decl, hasUnknownAttr) {
        auto unkownAttr = Node.getAttr<clang::UnknownAttr>();
        if (unkownAttr != nullptr) {
            return true;
        } else {
            return false;
        }
    }

    // Returns true if the declaration has an unknown attribute
    AST_MATCHER(Decl, hasAnyAttr) { return Node.hasAttrs(); }

    // Returns true if the template is a variadic template
    AST_POLYMORPHIC_MATCHER(isVariadicTemplate, AST_POLYMORPHIC_SUPPORTED_TYPES_2(FunctionTemplateDecl, ClassTemplateDecl)) {
        for(auto param : Node.getTemplateParameters()->asArray()) {
            if (param->isParameterPack()) {
                return true;
            }
        }
        return false;
    }

    // Returns true if the return type is template dependent
    AST_POLYMORPHIC_MATCHER(hasDependentReturnType, AST_POLYMORPHIC_SUPPORTED_TYPES_2(FunctionDecl, FunctionTemplateDecl)) {
        const auto& retType = Node.getAsFunction()->getCanonicalDecl()->getReturnType().getCanonicalType().getTypePtrOrNull();
        if ((retType != nullptr) && (retType->isDependentType())) {
            return true;
        } else {
            return false;
        }
    }

    // Returns true if the function is a definition with null body (it's never called)
    AST_POLYMORPHIC_MATCHER(isUnusedFunctionDecl, AST_POLYMORPHIC_SUPPORTED_TYPES_2(FunctionDecl, FunctionTemplateDecl)) {
        if ((Node.getAsFunction()->getBody() == nullptr) && (Node.getAsFunction()->isThisDeclarationADefinition())) {
            return true;
        } else {
            return false;
        }
    }

    // Returns true if the provided FunctionDecl has a trailing return type.
    AST_POLYMORPHIC_MATCHER(hasTrailingReturnType, AST_POLYMORPHIC_SUPPORTED_TYPES_2(FunctionDecl, FunctionTemplateDecl)) {
        if (auto FType = Node.getAsFunction()->getFunctionType()) {
            if (auto FProtoType = FType->template getAs<FunctionProtoType>()) {
                return FProtoType->hasTrailingReturn();
            }
        }
        return false;
    }

    // Custom matcher to check if function is declaration only
    AST_POLYMORPHIC_MATCHER(isDeclarationOnly, AST_POLYMORPHIC_SUPPORTED_TYPES_2(FunctionDecl, FunctionTemplateDecl)) {
        if (!Node.getAsFunction()->isThisDeclarationADefinition()) {
            return true;
        } else {
            return false;
        }
    }

    // Custom matcher to check if function is a template specialization.
    SIMPLE_MATCHER_WITH_ACTION_DECL(isTemplateSpecialization, FunctionDecl, isFunctionTemplateSpecialization)

    // Returns true when the given node contains a template type aliased type in typedefs.
    SIMPLE_MATCHER_DECL(typedefDeclTypeContainsTemplateTypeAlias, TypedefDecl, backport::helper::isTemplateTypeAliasedTypeOrFunctionType(Node.getUnderlyingType()))

    // Returns true when the given node contains a template type aliased type in class template specialization.
    SIMPLE_MATCHER_DECL(classTemplatespecializationContainsTemplateTypeAlias, ClassTemplateSpecializationDecl, backport::helper::isTemplateTypeAliasedTypeOrFunctionType(Node.getTypeAsWritten()->getType()))

    // Returns true when the given node contains a template type aliased type.
    SIMPLE_MATCHER_DECL(valueDeclTypeContainsTemplateTypeAlias, ValueDecl, backport::helper::isTemplateTypeAliasedTypeOrFunctionType(Node.getType()))

    // Returns true when the given node contains a template type aliased type in function return type.
    SIMPLE_MATCHER_DECL(returnTypeContainsTypeAlias, FunctionDecl, backport::helper::isTemplateTypeAliasedTypeOrFunctionType(Node.getReturnType()))

    // Returns true when the given node is out of line.
    SIMPLE_MATCHER_DECL(isOutOfLine, FunctionDecl, Node.isOutOfLine())

    // Returns ture when the given method defition contains type alias qualifier.
    AST_MATCHER(CXXMethodDecl, methodNameContainsTypeAlias) {
        if (Node.getQualifierLoc().hasQualifier() && !Node.getQualifierLoc().getTypeLoc().getBeginLoc().isInvalid()) {
            return backport::helper::isTemplateTypeAliasedTypeOrFunctionType(Node.getQualifierLoc().getTypeLoc().getType());
        }
        return false;
    }

    // Returns true when the given node contains a template type aliased type in template specialization.
    AST_MATCHER(FunctionDecl, functionDeclContainsTypeAlias) {
        bool contains = false;
        if (!Node.getTemplateSpecializationArgsAsWritten()) {
            return false;
        }
        for (unsigned int i = 0; i < Node.getTemplateSpecializationArgsAsWritten()->NumTemplateArgs; ++i) {
            auto &ASTTemplateSpecArgs = (*Node.getTemplateSpecializationArgsAsWritten());

            // Only do this for types.
            if (ASTTemplateSpecArgs[i].getArgument().getKind() == clang::TemplateArgument::ArgKind::Type) {
                bool contains = backport::helper::isTemplateTypeAliasedTypeOrFunctionType(ASTTemplateSpecArgs[i].getTypeSourceInfo()->getType());

                if (contains)
                    return true;
            }
        }

        return false;
    }

    // Returns true when the given expression contains a template type aliased type.
    AST_MATCHER(Expr, exprContainsTypeAlias) {
        QualType type;
        if (auto unaryexpr = dyn_cast<UnaryExprOrTypeTraitExpr>(&Node)) {
            type = unaryexpr->getTypeOfArgument();
        }
        else if (auto explCast = dyn_cast<ExplicitCastExpr>(&Node)) {
            type = explCast->getTypeAsWritten();
        }
        else if (auto unresolvedConstruct = dyn_cast<CXXUnresolvedConstructExpr>(&Node)) {
            type = unresolvedConstruct->getTypeAsWritten();
        }
        else {
            type = Node.getType();
        }
        return backport::helper::isTemplateTypeAliasedTypeOrFunctionType(type);
    }

    // Returns true if default template parameter contains type alias.
    SIMPLE_MATCHER_DECL(templateParmDeclcontainsTypeAlias, TemplateTypeParmDecl, backport::helper::isTemplateTypeAliasedTypeOrFunctionType(Node.getDefaultArgument()))

    // Custom matcher which casts to template type parm decl
    AST_DECL_MATCHER(TemplateTypeParmDecl, templateTypeParmDecl)

    // Custom matcher which casts to template type parm decl
    AST_DECL_MATCHER(NonTypeTemplateParmDecl, nonTypeTemplateParmDecl)

    // Returns true if the template parameter has a default argument.
    SIMPLE_MATCHER_WITH_ACTION_DECL(hasDefaultArgument, TemplateTypeParmDecl, hasDefaultArgument)

    AST_MATCHER_P(TemplateTypeParmDecl, hasDefaultArgument, internal::Matcher<QualType>, InnerMatcher) {
        if (!Node.hasDefaultArgument())
            return false;

        return InnerMatcher.matches(Node.getDefaultArgument(), Finder, Builder);
    }

    // Move from function declaration to function body
    AST_POLYMORPHIC_MATCHER_P(hasFunctionBody, AST_POLYMORPHIC_SUPPORTED_TYPES_2(FunctionDecl, FunctionTemplateDecl), internal::Matcher<Stmt>, InnerMatcher) {
        const Stmt *const Statement = Node.getBody();
        return (Statement != nullptr && InnerMatcher.matches(*Statement, Finder, Builder));
    }

    // Move from function template specialization to the generic function template
    AST_MATCHER_P(FunctionDecl, hasFunctionTemplate, internal::Matcher<FunctionTemplateDecl>, InnerMatcher) {
        const FunctionTemplateDecl *const FuncDecl = Node.getPrimaryTemplate();
        return (FuncDecl != nullptr && InnerMatcher.matches(*FuncDecl, Finder, Builder));
    }

    // Moves from constructor node to record declaration node.
    AST_MATCHER_P(CXXConstructorDecl, hasClassDecl, internal::Matcher<CXXRecordDecl>, InnerMatcher) {
        const CXXRecordDecl *const ClassDecl = Node.getParent();
        return (ClassDecl != nullptr && InnerMatcher.matches(*ClassDecl, Finder, Builder));
    }

    // Custom matcher for TranslationUnitDecl
    SIMPLE_MATCHER_DECL_DIFF_TYPE(isTranslationUnit, Decl, clang::TranslationUnitDecl)

    // Custom matcher for TypeAliasDecl
    AST_DECL_MATCHER(TypeAliasDecl, typeAliasDecl);

    // Custom matcher for TypeAliasTemplateDecl
    AST_DECL_MATCHER(TypeAliasTemplateDecl, typeAliasTemplateDecl);

    // Matches a QualType if it contains a given subtype.
    AST_MATCHER_P(QualType, hasSubTypeMatcher, internal::Matcher<QualType>, InnerMatcher) {
        QualType T = *dyn_cast<clang::QualType>(&Node);

        auto p = std::bind(&internal::Matcher<QualType>::matches, InnerMatcher, std::placeholders::_1, Finder, Builder);
        return backport::helper::hasSubType(T, p);
    }

    // Matches a Type if it is a decltype
    SIMPLE_MATCHER_DECL(isThisADecltype, QualType, T->getTypePtr() != nullptr && dyn_cast<DecltypeType>(T->getTypePtr()) != nullptr)

    // Matches a Typeloc if it contains a decltype.
    SIMPLE_MATCHER_DECL(isThisTypeLocContainsADecltype, TypeLoc, hasSubTypeMatcher(isThisADecltype()).matches(T->getType(), Finder, Builder))

    // Matches a QualType if it is a dependent type
    SIMPLE_MATCHER_DECL(isDependentQualType, QualType, T->getTypePtr() != nullptr && T->getTypePtr()->isDependentType())

    // Matches a type if it is a dependent type
    SIMPLE_MATCHER_WITH_ACTION_DECL(isDependentType, Type, isDependentType)

    // Matches a QualType if it contains a decltype.
    AST_MATCHER(QualType, doesThisQualTypeContainsADecltype) {
        auto RT = dyn_cast<clang::QualType>(&Node);

        if (RT == nullptr)
            return false;

        if (RT->isNull())
            return false;

        auto T = dyn_cast<clang::DecltypeType>(RT->getTypePtr());

        return T != nullptr || hasSubTypeMatcher(isThisADecltype()).matches(*RT, Finder, Builder);
    }

    // Matches a ReturnStmt if the type of it is a moveable class.
    AST_MATCHER(ReturnStmt, hasUserDeclaredMoveConstr) {
        auto T = dyn_cast<ReturnStmt>(&Node);
        auto cxxRecordD = T->getRetValue()->getType().getTypePtr()->getAsCXXRecordDecl();

        return cxxRecordD != nullptr && cxxRecordD->hasUserDeclaredMoveConstructor();
    }

    // Matches a methoddecl if it is the default constructor.
    SIMPLE_MATCHER_WITH_ACTION_DECL_DIFF_TYPE(isDefaultConstr, CXXMethodDecl, CXXConstructorDecl, isDefaultConstructor)

    // Matches a methoddecl if it is the move constructor.
    SIMPLE_MATCHER_WITH_ACTION_DECL_DIFF_TYPE(isMoveConstr, CXXMethodDecl, CXXConstructorDecl, isMoveConstructor)

    // Matches a methoddecl if it is the copy assigment overload operator.
    SIMPLE_MATCHER_WITH_ACTION_DECL(isCopyAssign, CXXMethodDecl, isCopyAssignmentOperator)

    // Matches a methoddecl if it is the move assignment overlad operator.
    SIMPLE_MATCHER_WITH_ACTION_DECL(isMoveAssign, CXXMethodDecl, isMoveAssignmentOperator)

    // Matches a methoddecl if it is the copy constructor.
    SIMPLE_MATCHER_WITH_ACTION_DECL_DIFF_TYPE(isCopyConstr, CXXMethodDecl, CXXConstructorDecl, isCopyConstructor)

    // Matches a methoddecl if it is marked as used.
    SIMPLE_MATCHER_WITH_ACTION_DECL(isUsed, CXXMethodDecl, isUsed)

    // Matches a ReturnStmt if the expr that is being returned matches the given matchers.
    SIMPLE_MATCHER_WITH_ACTION_WITH_PARAM_DECL(retValExpr, ReturnStmt, ReturnStmt, Expr, getRetValue)

    // Matches a recorddecl if it is a trivial (c++11) class.
    SIMPLE_MATCHER_WITH_ACTION_DECL(isTrivial, CXXRecordDecl, isTrivial)

    // Matches an expression if the value type of it is Xvalue(c++11).
    SIMPLE_MATCHER_WITH_ACTION_DECL(isXValue, Expr, isXValue)

    // Matches an expression if the type of it is class type with user declared move constructor.
    AST_MATCHER(Expr, hasTypeWithUserDeclaredMoveConstr) {
        auto T = dyn_cast<Expr>(&Node);

        auto cxxRecordD = T->getType().getTypePtr()->getAsCXXRecordDecl();

        return cxxRecordD != nullptr && cxxRecordD->hasUserDeclaredMoveConstructor();
    }

    // Matches an expression if the type of it is class type with user declared move assignment operator overload.
    AST_MATCHER(Expr, hasTypeWithUserDeclaredMoveAssign) {
        auto T = dyn_cast<Expr>(&Node);

        auto cxxRecordD = T->getType().getTypePtr()->getAsCXXRecordDecl();

        return cxxRecordD != nullptr && cxxRecordD->hasUserDeclaredMoveAssignment();
    }

    // Matches an expression if the type of it is class type and not a trivial class(c++11).
    AST_MATCHER(Expr, hasNonTrivialCXXRecordType) {
        auto T = dyn_cast<Expr>(&Node);

        auto cxxRecordD = T->getType().getTypePtr()->getAsCXXRecordDecl();

        return cxxRecordD != nullptr && cxxRecordD->isTrivial() != true;
    }

    // Tries to match template functions which have universal/forwarding reference parameters.
    AST_MATCHER(FunctionTemplateDecl, hasForwardingReferenceParam) {
        auto T = dyn_cast<FunctionTemplateDecl>(&Node);

        if (T == nullptr)
            return false;

        for (auto param : T->getTemplatedDecl()->parameters()) {
            if (param->getOriginalType()->isRValueReferenceType() && 
                param->getOriginalType()->isDependentType())
                return true;
        }

        return false;
    }

    // Matches a cast expression if it is cast from derived to base class type.
    SIMPLE_MATCHER_DECL(hasCastKindDerivedToBase, CastExpr, T->getCastKind() == clang::CastKind::CK_DerivedToBase)

    // Matches a class template if it has a specialization which matches the given matcher.
    AST_MATCHER_P(ClassTemplateDecl, hasSpecialization, internal::Matcher<ClassTemplateSpecializationDecl>, InnerMatcher) {
        auto T = dyn_cast<ClassTemplateDecl>(&Node);

        for (auto s : T->specializations()) {
            if (InnerMatcher.matches(*s, Finder, Builder))
                return true;
        }

        return false;
    }

    // Matches a template argument if the type of the argument is matches the given matcher.
    AST_MATCHER_P(TemplateArgument, hasTypeTemplateArg, internal::Matcher<QualType>, InnerMatcher) {
        auto T = dyn_cast<TemplateArgument>(&Node);

        return T != nullptr && T->getKind() == TemplateArgument::Type && InnerMatcher.matches(T->getAsType(), Finder, Builder);
    }

    // Matches a typedef if the underlying type matches the given matcher.
    AST_MATCHER_P(TypedefNameDecl, hasUnderlyingType, internal::Matcher<QualType>, InnerMatcher) {
        const QualType type = Node.getUnderlyingType();
        return (InnerMatcher.matches(type, Finder, Builder));
    }

    // Returns true if the current VarDecl node is in a template instantiation.
    AST_MATCHER(VarDecl, isInInstantiation) {
        if (Node.getParentFunctionOrMethod()) {
            if (auto func = dyn_cast<FunctionDecl>(Node.getParentFunctionOrMethod())) {
                if (func->getTemplateSpecializationKind() == TSK_ImplicitInstantiation || func->getTemplateSpecializationKind() == TSK_ExplicitInstantiationDefinition) {
                    return true;
                }
            }
        }
        return false;
    }
} /* namespace ast_matchers */ } /* namespace clang */

#endif // BACKPORT_CUSTOM_MATCHERS
